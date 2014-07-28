#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <iostream>

#include "bluetooth/bluetooth.h"
#include "bluetooth/hci.h"
#include "bluetooth/hci_lib.h"

#include "AndroidBluetooth.h"
#include "BluetoothHCI.h"
#include "Logger.h"
#include "Timing.h"

using namespace std;

static int rfkill_id = -1;
static char *rfkill_state_path = NULL;

static int init_rfkill()
{
  char path[64];
  char buf[16];
  for(int id = 0;; id++)
  {
    snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type", id);
    const int fd = open(path, O_RDONLY);
    if(fd < 0)
    {
      throw BluetoothHCIException("init rfkill: open(" + string(path) + ") failed");
    }
    const int sz = read(fd, &buf, sizeof(buf));
    close(fd);
    if(sz >= 9 && memcmp(buf, "bluetooth", 9) == 0)
    {
      rfkill_id = id;
      break;
    }
  }
  asprintf(&rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", rfkill_id);
}

static bool check_bluetooth_power()
{
  if(rfkill_id == -1)
  {
    init_rfkill();
  }

  const int fd = open(rfkill_state_path, O_RDONLY);
  if(fd < 0)
  {
    throw BluetoothHCIException("open(" + string(rfkill_state_path) + ") failed");
  }

  char buffer;
  const int sz = read(fd, &buffer, 1);
  if(sz != 1)
  {
    close(fd);
    throw BluetoothHCIException("read(" + string(rfkill_state_path) + ") failed");
  }
  close(fd);

  switch(buffer)
  {
  case '1':
    return true;
  case '0':
    return false;
  }
}

static void set_bluetooth_power(const bool on)
{
  const char buffer = (on ? '1' : '0');

  if(rfkill_id == -1)
  {
    init_rfkill();
  }

  const int fd = open(rfkill_state_path, O_WRONLY);
  if(fd < 0)
  {
    throw BluetoothHCIException("open(" + string(rfkill_state_path) + ") for write failed");
  }

  const int sz = write(fd, &buffer, 1);
  if(sz < 0)
  {
    close(fd);
    throw BluetoothHCIException("write(" + string(rfkill_state_path) + ") failed");
  }
  close(fd);
}

static inline int create_hci_sock()
{
  const int sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
  if(sk < 0)
  {
    throw BluetoothHCIException("Failed to create bluetooth HCI raw socket");
  }
  return sk;
}

void startHCIAttach()
{
  if(property_set("ctl.start", "hciattach") < 0)
  {
    set_bluetooth_power(false);
    throw BluetoothHCIException("Failed to start hciattach");
  }
}

void stopHCIAttach()
{
  LOG_D("AndroidBluetooth", "Stopping hciattach daemon");
  if(property_set("ctl.stop", "hciattach") < 0)
  {
    throw BluetoothHCIException("Error stopping hciattach");
  }
}

void ioctlDown(const int adapterID)
{
  const int hci_sock = create_hci_sock();
  LOG_D("AndroidBluetooth", "ioctl HCIDEVDOWN");
  if(ioctl(hci_sock, HCIDEVDOWN, adapterID) < 0)
  {
    LOG_D("AndroidBluetooth", "ioctl HCIDEVDOWN failed");
  }
  close(hci_sock);
}

void ioctlUp(const int adapterID)
{
  const int hci_sock = create_hci_sock();
  LOG_D("AndroidBluetooth", "ioctl HCIDEVUP");
  if(ioctl(hci_sock, HCIDEVUP, adapterID) < 0)
  {
    close(hci_sock);
    throw BluetoothHCIException("ioctl HCIDEVUP failed");
  }
  close(hci_sock);
}

void startBluetoothd()
{
  LOG_D("AndroidBluetooth", "Starting bluetoothd daemon");
  if(property_set("ctl.start", "bluetoothd") < 0)
  {
    //ioctlDown(0); // TODO adapterID
    stopHCIAttach();
    set_bluetooth_power(false);
    throw BluetoothHCIException("Failed to start bluetoothd");
  }
}

void stopBluetoothd()
{
  LOG_D("AndroidBluetooth", "Stopping bluetoothd daemon");
  if(property_set("ctl.stop", "bluetoothd") < 0)
  {
    throw BluetoothHCIException("Error stopping bluetoothd");
  }
  sleepMS(500);
}

void bt_enable(const int adapterID)
{
  set_bluetooth_power(true);
  startHCIAttach();

  // Try for 10 seconds, this can only succeed once hciattach has sent the
  // firmware and then turned on hci device via HCIUARTSETPROTO ioctl
  int attempt;
  for(attempt = 100; attempt > 0;  attempt--) // Nexus S: 30, Galaxy Nexus: 10
  {
    try
    {
      ioctlUp(adapterID);
      break;
    }
    catch(BluetoothHCIException ex)
    {
      if(errno == EALREADY)
      {
        break;
      }
      else
      {
        sleepMS(250);
      }
    }
  }

  if(attempt == 0)
  {
    stopHCIAttach();
    set_bluetooth_power(false);
    throw BluetoothHCIException("[CRASH] Timeout waiting for HCI device to come up");
  }

  startBluetoothd();
  sleepSec(1);
  if(!bt_is_enabled(adapterID))
  {
    throw BluetoothHCIException("[CRASH] HCI device not available at the end of the sequence");
  }
}

void bt_disable(const int adapterID)
{
  stopBluetoothd();
  stopHCIAttach();
  set_bluetooth_power(false);
  sleepSec(1);
}

bool bt_is_enabled(const int adapterID)
{
  // Check power first
  if(check_bluetooth_power() == false) return false;

  // Power is on, now check if the HCI interface is up
  const int hci_sock = create_hci_sock();
  struct hci_dev_info dev_info;
  memset(&dev_info, 0, sizeof(dev_info));

  dev_info.dev_id = adapterID;
  if(ioctl(hci_sock, HCIGETDEVINFO, (void *)&dev_info) < 0)
  {
    close(hci_sock);
    return false;
  }
  else
  {
    close(hci_sock);
  }
  if(dev_info.flags & (1 << (HCI_UP & 31)))
  {
    return true;
  }
  else
  {
    return false;
  }
}
