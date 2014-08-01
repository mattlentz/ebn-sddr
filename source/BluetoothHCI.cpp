#include "BluetoothHCI.h"

#include <cerrno>
#include <fcntl.h>
#include <future>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#include <sstream>

#include "bluetooth/hci_lib.h"
#include "bluetooth/l2cap.h"
#include "bluetooth/rfcomm.h"

#include "AndroidBluetooth.h"
#include "Logger.h"
#include "Timing.h"

using namespace std;

void LOG_E_BT_CRASH(const std::string &tag, const std::string &msg, const std::string &file,
                    const int line, const int errorCode)
{
  stringstream ss;
  ss << msg << " (Error " << errorCode << ": " << strerror(errorCode) << ") at " << file << ":" << line;
  LOG_E(tag.c_str(), ss.str().c_str());
  throw BluetoothHCIException(ss.str());
}

BluetoothHCI::BluetoothHCI(int adapterID)
   : adapterID_(adapterID),
     sock_(-1),
     lastKnownState_()
{
  sock_ = hci_open_dev(adapterID);
  if(sock_ < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  readState();
}

BluetoothHCI::~BluetoothHCI()
{
  // Turning off advertising if it was enabled
  if(lastKnownState_.isAdvertisingStored && lastKnownState_.isAdvertising)
  {
    enableAdvertising(false);
  }

  if(sock_ != -1)
  {
    hci_close_dev(sock_);
  }
}

void BluetoothHCI::readState()
{
  struct hci_dev_info deviceInfo;
  memset(&deviceInfo, 0, sizeof(deviceInfo));

  int error;
  if((error = hci_devinfo(adapterID_, &deviceInfo)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  if(!hci_test_bit(HCI_UP, &deviceInfo.flags))
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Need to bring up device", __FILE__, __LINE__, 0);
    //readState();
    return;
  }

  lastKnownState_.publicAddress = Address(6, deviceInfo.bdaddr.b);
  lastKnownState_.publicAddressStored = true;

  lastKnownState_.isConnectable = (hci_test_bit(HCI_PSCAN, &deviceInfo.flags) != 0);
  lastKnownState_.isConnectableStored = true;

  lastKnownState_.isDiscoverable = (hci_test_bit(HCI_ISCAN, &deviceInfo.flags) != 0);
  lastKnownState_.isDiscoverableStored = true;
}

void BluetoothHCI::setConnectable(bool value)
{
  lastKnownState_.isConnectable = value;
  lastKnownState_.isConnectableStored = true;

  setScan(lastKnownState_.isConnectable, lastKnownState_.isDiscoverable);
}

void BluetoothHCI::setDiscoverable(bool value)
{
  lastKnownState_.isDiscoverable = value;
  lastKnownState_.isDiscoverableStored = true;

  setScan(lastKnownState_.isConnectable, lastKnownState_.isDiscoverable);
}

void BluetoothHCI::setInquiryMode(InquiryMode value)
{
  lastKnownState_.inquiryMode = value;
  lastKnownState_.inquiryModeStored = true;

  int error;
  if((error = hci_write_inquiry_mode(sock_, value, 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }
}

Address BluetoothHCI::getPublicAddress()
{
  bdaddr_t address;
  int error;

  if((error = hci_read_bd_addr(sock_, &address, 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  return Address(6, address.b);
}

Address BluetoothHCI::getRandomAddress()
{
  // There is no command to actually read the random address, so we must
  // rely on the last known value (if stored)
  if(!lastKnownState_.randomAddressStored)
  {
    return Address(6, NULL);
  }

  return lastKnownState_.randomAddress;
}

void BluetoothHCI::setPublicAddress(const Address &address)
{
  lastKnownState_.publicAddress = address;
  lastKnownState_.publicAddressStored = true;

  // TODO: NOT compatible across systems with different host byte orders
  Address swappedAddress = address.swap();

  close(sock_);
  bt_disable(adapterID_);
  system(("echo \"" + swappedAddress.toString() + "\" > `getprop ro.bt.bdaddr_path`").c_str());
  while(true)
  {
    try
    {
      bt_enable(adapterID_);
      break;
    }
    catch(BluetoothHCIException ex)
    {
      LOG_D("BluetoothHCI", "Failed to enable adapter %d - %s", adapterID_, ex.what());
      bt_disable(adapterID_);
    }
  }

  sock_ = hci_open_dev(adapterID_);
  if(sock_ < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  LOG_P("BluetoothHCI", "Changed public address to %s", address.toString().c_str());
}

void BluetoothHCI::setRandomAddress(const Address &address)
{
  struct hci_request request;
  le_set_random_address_cp params;
  uint8_t status;

  memset(&request, 0, sizeof(request));
  memset(&params, 0, sizeof(params));

  memcpy(params.bdaddr.b, address.toByteArray(), 6);

  request.ogf = OGF_LE_CTL;
  request.ocf = OCF_LE_SET_RANDOM_ADDRESS;
  request.cparam = &params;
  request.clen = sizeof(params);
  request.rparam = &status;
  request.rlen = 1;

  int error;
  if((error = hci_send_req(sock_, &request, 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  lastKnownState_.randomAddress = address;
  lastKnownState_.randomAddressStored = true;

  LOG_P("BluetoothHCI", "Changed random address to %s", address.toString().c_str());
}

string BluetoothHCI::readLocalName()
{
  char name[HCI_MAX_NAME_LENGTH] = { 0 };
  int error;
  if((error = hci_read_local_name(sock_, HCI_MAX_NAME_LENGTH, name, 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  return string(name);
}

bool BluetoothHCI::readRemoteName(string &name, const Address &address, uint16_t clockOffset, uint8_t pageScanMode, uint64_t timeout)
{
  char remoteName[HCI_MAX_NAME_LENGTH] = { 0 };

  bdaddr_t addressBT;
  memcpy(addressBT.b, address.toByteArray(), 6);

  int error;
  if((error = hci_read_remote_name_with_clock_offset(sock_, &addressBT, pageScanMode, clockOffset, HCI_MAX_NAME_LENGTH, remoteName, timeout)) < 0)
  {
    // Connection timed out or I/O error are not fatal, just return that
    // we failed to complete the name request
    if((errno == 110) || (errno == 5))
    {
      return false;
    }
    else
    {
      LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
    }
  }

  name = string(remoteName);

  return true;
}

void BluetoothHCI::writeLocalName(string name)
{
  int error;
  if((error = hci_write_local_name(sock_, name.c_str(), 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }
}

void BluetoothHCI::writeExtInquiryResponse(uint8_t *data)
{
  // TODO: Check if this holds after hardware reset, or if we need to keep
  // track of it ourselves in the last known state

  int error;
  if((error = hci_write_ext_inquiry_response(sock_, 0, data, 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }
}

list<InquiryResponse> BluetoothHCI::performInquiry(int periods)
{
  LOG_D("BluetoothHCI", "Starting discovery for %d periods...", periods);

  inquiry_info *rawResponses = new inquiry_info[255];
  int numResponses = hci_inquiry(adapterID_, periods, 255, NULL, &rawResponses, IREQ_CACHE_FLUSH);
  if(numResponses < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  LOG_D("BluetoothHCI", "Finished discovery. Found %d devices.", numResponses);

  list<InquiryResponse> responses;
  for(int r = 0; r < numResponses; r++)
  {
    InquiryResponse response;
    response.address = Address(6, rawResponses[r].bdaddr.b);
    response.clockOffset = rawResponses[r].clock_offset;
    response.pageScanMode = rawResponses[r].pscan_rep_mode;
    response.pageScanPeriodMode = rawResponses[r].pscan_period_mode;

    responses.push_back(response);
  }

  delete rawResponses;

  return responses;
}

void BluetoothHCI::performEIRInquiry(function<void(const EIRInquiryResponse *)> callback, int periods)
{
  int sockEIR = hci_open_dev(adapterID_);
  if(sockEIR < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  struct hci_filter filter;
  hci_filter_clear(&filter);
  hci_filter_set_ptype(HCI_EVENT_PKT, &filter);
  hci_filter_set_event(EVT_EXTENDED_INQUIRY_RESULT, &filter);

  int error;
  if((error = setsockopt(sockEIR, SOL_HCI, HCI_FILTER, &filter, sizeof(filter))) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  inquiry_info *rawResponses = new inquiry_info[255];
  auto handle = async(launch::async, hci_inquiry, adapterID_, periods, 255, (const uint8_t *)NULL, &rawResponses, IREQ_CACHE_FLUSH);

  uint8_t buffer[HCI_MAX_EVENT_SIZE];
  uint64_t startTime = getTimeMS();
  int64_t remainingTime;

  while((remainingTime = (periods * 1280) - (getTimeMS() - startTime)) >= 0)
  {
    struct pollfd pollDesc;
    pollDesc.fd = sockEIR;
    pollDesc.events = POLLIN;

    error = poll(&pollDesc, 1, remainingTime);
    if(error == -1)
    {
      LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
      break;
    }
    else if(error == 0)
    {
      break;
    }

    int len = ::recv(sockEIR, buffer, sizeof(buffer), MSG_DONTWAIT);
    if(len > 0)
    {
      extended_inquiry_info *eii = (extended_inquiry_info *)(buffer + 2 + HCI_EVENT_HDR_SIZE);

      EIRInquiryResponse response;
      response.address = Address(6, eii->bdaddr.b);
      response.clockOffset = eii->clock_offset;
      response.pageScanMode = eii->pscan_rep_mode;
      response.pageScanPeriodMode = eii->pscan_period_mode;
      response.data = eii->data;
      response.rssi = eii->rssi;

      callback(&response);
    }
    else
    {
      break;
    }
  }

  if(!handle.valid() || (handle.get() < 0))
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  delete rawResponses;

  hci_close_dev(sockEIR);
}

void BluetoothHCI::performScan(Scan type, DuplicateFilter filter, uint64_t duration, function<void(const ScanResponse *)> callback)
{
  LOG_D("BluetoothHCI", "Performing a scan");
  const int sockScan = hci_open_dev(adapterID_);
  if(sockScan < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  struct hci_filter eventFilter;
  hci_filter_clear(&eventFilter);
  hci_filter_set_ptype(HCI_EVENT_PKT, &eventFilter);
  hci_filter_set_event(EVT_LE_META_EVENT, &eventFilter);

  int error;
  if((error = setsockopt(sockScan, SOL_HCI, HCI_FILTER, &eventFilter, sizeof(eventFilter))) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  if((error = hci_le_set_scan_parameters(sock_, type, htobs((uint16_t)(duration / 0.625)), htobs((uint16_t)(duration / 0.625)), LE_RANDOM_ADDRESS, ScanFilter::All, 0)) < 0)
  {
    // EIO is returned in the case that a scan was already running. We should just disable scanning and rerun
    if(errno == EIO)
    {
      LOG_W("BluetoothHCI", "Scan was already running... Disabling, then starting new scan");

      hci_le_set_scan_enable(sock_, 0, 0, 0);
      if((error = hci_le_set_scan_parameters(sock_, type, htobs((uint16_t)(duration / 0.625)), htobs((uint16_t)(duration / 0.625)), LE_RANDOM_ADDRESS, ScanFilter::All, 0)) < 0)
      {
        LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
      }
    }
    else
    {
      LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
    }
  }

  if((error = hci_le_set_scan_enable(sock_, 1, filter, 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  uint8_t buffer[HCI_MAX_EVENT_SIZE];
  uint64_t startTime = getTimeMS();
  int64_t remainingTime;

  while((remainingTime = duration - (getTimeMS() - startTime)) >= 0)
  {
    struct pollfd pollDesc;
    pollDesc.fd = sockScan;
    pollDesc.events = POLLIN;

    error = poll(&pollDesc, 1, remainingTime);
    if(error == -1)
    {
      LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
      break;
    }
    else if(error == 0)
    {
      break;
    }

    ::recv(sockScan, buffer, sizeof(buffer), MSG_DONTWAIT);

    hci_event_hdr *eventHeader = (hci_event_hdr *)(buffer + 1);
    void *eventBody = (void *)(buffer + 1 + HCI_EVENT_HDR_SIZE);

    if(eventHeader->evt == EVT_LE_META_EVENT)
    {
      const evt_le_meta_event *metaEventHeader = (evt_le_meta_event *)(eventBody);
      const uint8_t *metaEventBody = metaEventHeader->data;

      //Bluetooth LE advertising report event
      if(metaEventHeader->subevent == 0x02)
      {
        uint8_t numReports = *(metaEventBody++);

        for(int r = 0; r < numReports; r++)
        {
          le_advertising_info *info = (le_advertising_info *)(metaEventBody);

          ScanResponse response;
          response.address = Address(6, info->bdaddr.b);
          response.data = info->data;
          response.length = info->length;
          response.rssi = ((int8_t *)info->data)[info->length];

          callback(&response);

          metaEventBody += LE_ADVERTISING_INFO_SIZE + info->length + 1;
        }
      }
    }
  }

  if((error = hci_le_set_scan_enable(sock_, 0, 0, 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }
  close(sockScan);
  LOG_D("BluetoothHCI", "Scan finished");
}

void BluetoothHCI::setAdvertData(const uint8_t *data, uint8_t length)
{
  struct hci_request req;
  le_set_advertising_data_cp param;
  uint8_t status;

  memset(&req, 0, sizeof(req));
  memset(&param, 0, sizeof(param));

  param.length = length;
  memcpy(param.data, data, length);

  req.ogf = OGF_LE_CTL;
  req.ocf = OCF_LE_SET_ADVERTISING_DATA;
  req.cparam = &param;
  req.clen = sizeof(param);
  req.rparam = &status;
  req.rlen = 1;

  int error;
  if((error = hci_send_req(sock_, &req, 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  lastKnownState_.advertData = param;
  lastKnownState_.advertDataStored = true;
}

void BluetoothHCI::setScanResponseData(const uint8_t *data, uint8_t length)
{
  struct hci_request req;
  le_set_scan_response_data_cp param;
  uint8_t status;

  memset(&req, 0, sizeof(req));
  memset(&param, 0, sizeof(param));

  param.length = length;
  memcpy(param.data, data, length);

  req.ogf = OGF_LE_CTL;
  req.ocf = OCF_LE_SET_SCAN_RESPONSE_DATA;
  req.cparam = &param;
  req.clen = sizeof(param);
  req.rparam = &status;
  req.rlen = 1;

  int error;
  if((error = hci_send_req(sock_, &req, 0)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  lastKnownState_.responseData = param;
  lastKnownState_.responseDataStored = true;
}

void BluetoothHCI::setUndirectedAdvertParams(UndirectedAdvert type, AdvertFilter filter, uint16_t minInterval, uint16_t maxInterval)
{
  struct hci_request req;
  le_set_advertising_parameters_cp param;
  uint8_t status;

  memset(&req, 0, sizeof(req));
  memset(&param, 0, sizeof(param));

  param.min_interval = htobs((uint16_t)(minInterval / 0.625));
  param.max_interval = htobs((uint16_t)(maxInterval / 0.625));
  param.advtype = type;
  param.own_bdaddr_type = LE_RANDOM_ADDRESS;
  param.chan_map = ChannelMap::All;
  param.filter = filter;

  enableAdvertising(false);

  req.ogf = OGF_LE_CTL;
  req.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
  req.cparam = &param;
  req.clen = sizeof(param);
  req.rparam = &status;
  req.rlen = 1;

  int error;
  if(((error = hci_send_req(sock_, &req, 0)) < 0) || (error = status))
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  lastKnownState_.advertParams = param;
  lastKnownState_.advertParamsStored = true;

  enableAdvertising(true);
}

void BluetoothHCI::enableAdvertising(bool enable)
{
  LOG_D("BluetoothHCI", "Setting advertising to %d", enable);
  int error;
  if((error = hci_le_set_advertise_enable(sock_, enable, 0)) < 0)
  {
    // EIO is returned in the case that advertising was already enabled/disabled
    if(errno == EIO)
    {
      LOG_W("BluetoothHCI", "Advertising was already %s", enable ? "enabled" : "disabled");
    }
    else
    {
      LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
    }
  }

  lastKnownState_.isAdvertising = enable;
  lastKnownState_.isAdvertisingStored = true;
}

void BluetoothHCI::setScan(bool pscan, bool iscan)
{
  struct hci_dev_req request;
  memset(&request, 0, sizeof(request));

  request.dev_id  = adapterID_;
  request.dev_opt = SCAN_DISABLED;

  if(pscan && iscan)
  {
    request.dev_opt = SCAN_PAGE | SCAN_INQUIRY;
  }
  else if(pscan)
  {
    request.dev_opt = SCAN_PAGE;
  }
  else if(iscan)
  {
    request.dev_opt = SCAN_INQUIRY;
  }

  int error;
  if((error = ioctl(sock_, HCISETSCAN, (unsigned long)&request)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }
}

void BluetoothHCI::recover()
{
  LOG_E_BT_CRASH("BluetoothHCI", "recover() not implemented", __FILE__, __LINE__, 0);
}

int BluetoothHCI::connectBT2(const Address &address, uint8_t port, int64_t timeout)
{
  int error;

  int sockConn = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if(sockConn < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  int flags = fcntl(sockConn, F_GETFL, 0);
  if((error = fcntl(sockConn, F_SETFL, flags | O_NONBLOCK)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  struct sockaddr_rc addr = { 0 };
  addr.rc_family = AF_BLUETOOTH;
  memcpy(addr.rc_bdaddr.b, address.toByteArray(), 6);
  addr.rc_channel = port;

  if(((error = connect(sockConn, (struct sockaddr *)&addr, sizeof(addr))) < 0) && (errno != EINPROGRESS))
  {
    // EBUSY will occur when an incoming connection is already being handled,
    // and so we can safely ignore it
    if(errno != EBUSY)
    {
      LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
    }
    return -1;
  }

  // Waiting for a connection until timeout occurs, if the connection is still
  // in progress
  if(errno == EINPROGRESS)
  {
    struct pollfd pollDesc;
    pollDesc.fd = sockConn;
    pollDesc.events = POLLOUT;

    error = poll(&pollDesc, 1, timeout);
    if(error <= 0)
    {
      close(sockConn);
      sockConn = -1;
    }
  }

  return sockConn;
}

int BluetoothHCI::connectBT4(const Address &address, int64_t timeout)
{
  int error;

  int sockConn = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  if(sockConn < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  int flags = fcntl(sockConn, F_GETFL, 0);
  if((error = fcntl(sockConn, F_SETFL, flags | O_NONBLOCK)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  struct sockaddr_l2 addr = { 0 };
  addr.l2_family = AF_BLUETOOTH;
  memcpy(addr.l2_bdaddr.b, address.toByteArray(), 6);
  addr.l2_bdaddr_type = BDADDR_LE_RANDOM;
  addr.l2_cid = htobs(0x0004);

  if(((error = connect(sockConn, (struct sockaddr *)&addr, sizeof(addr))) < 0) && (errno != EINPROGRESS))
  {
    // EBUSY will occur when an incoming connection is already being handled,
    // and so we can safely ignore it
    if(errno != EBUSY)
    {
      LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
    }
    return -1;
  }

  // Waiting for a connection until timeout occurs, if the connection is still
  // in progress
  if(errno == EINPROGRESS)
  {
    struct pollfd pollDesc;
    pollDesc.fd = sockConn;
    pollDesc.events = POLLOUT;

    error = poll(&pollDesc, 1, timeout);
    if(error <= 0)
    {
      close(sockConn);
      sockConn = -1;
    }
  }

  return sockConn;
}

int BluetoothHCI::listenBT2(uint8_t port)
{
  int sockListen = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if(sockListen < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  struct sockaddr_rc addr = { 0 };
  addr.rc_family = AF_BLUETOOTH;
  addr.rc_channel = port;

  int error;
  if((error = bind(sockListen, (struct sockaddr *)&addr, sizeof(addr))) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  if((error = listen(sockListen, 5)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  int flags = fcntl(sockListen, F_GETFL, 0);
  if((error = fcntl(sockListen, F_SETFL, flags | O_NONBLOCK)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  return sockListen;
}

int BluetoothHCI::listenBT4()
{
  int sockListen = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  if(sockListen < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  struct sockaddr_l2 addr = { 0 };
  addr.l2_family = AF_BLUETOOTH;
  addr.l2_bdaddr_type = BDADDR_LE_RANDOM;
  addr.l2_cid = htobs(0x0004);

  int error;
  if((error = bind(sockListen, (struct sockaddr *)&addr, sizeof(addr))) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  if((error = listen(sockListen, 5)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  int flags = fcntl(sockListen, F_GETFL, 0);
  if((error = fcntl(sockListen, F_SETFL, flags | O_NONBLOCK)) < 0)
  {
    LOG_E_BT_CRASH("BluetoothHCI", "Recovery needed", __FILE__, __LINE__, errno);
  }

  return sockListen;
}

pair<int, Address> BluetoothHCI::acceptBT2(int sockListen)
{
  struct pollfd pollDesc;
  pollDesc.fd = sockListen;
  pollDesc.events = POLLIN;

  int error = poll(&pollDesc, 1, -1);
  if(error <= 0)
  {
    return make_pair(-1, Address());
  }
  else
  {
    struct sockaddr_rc addr;
    socklen_t addrLength = sizeof(sockaddr_rc);
    int sockClient = accept(sockListen, (struct sockaddr *)&addr, &addrLength);
    Address address;
    if(sockClient >= 0)
    {
      address = Address(6, addr.rc_bdaddr.b);
    }

    return make_pair(sockClient, address);
  }
}

pair<int, Address> BluetoothHCI::acceptBT4(int sockListen)
{
  struct pollfd pollDesc;
  pollDesc.fd = sockListen;
  pollDesc.events = POLLIN;

  int error = poll(&pollDesc, 1, -1);
  if(error <= 0)
  {
    return make_pair(-1, Address());
  }
  else
  {
    struct sockaddr_l2 addr;
    socklen_t addrLength = sizeof(sockaddr_l2);
    int sockClient = accept(sockListen, (struct sockaddr *)&addr, &addrLength);
    Address address;
    if(sockClient >= 0)
    {
      address = Address(6, addr.l2_bdaddr.b);
    }

    return make_pair(sockClient, address);
  }
}

bool BluetoothHCI::send(int sock, const uint8_t *message, size_t size, int64_t timeout)
{
  bool success = true;
  size_t pos = 0;
  uint64_t startTime = getTimeMS();
  int64_t remainingTime;

  while(success && (pos < size) && ((remainingTime = timeout - (getTimeMS() - startTime)) >= 0))
  {
    struct pollfd pollDesc;
    pollDesc.fd = sock;
    pollDesc.events = POLLOUT;

    int error = poll(&pollDesc, 1, remainingTime);
    if(error <= 0)
    {
      success = false;
    }
    else
    {
      ssize_t numWritten = write(sock, message + pos, size - pos);
      if(numWritten <= 0)
      {
        success = false;
      }
      else
      {
        pos += numWritten;
      }
    }
  }

  return success;
}

bool BluetoothHCI::recv(int sock, uint8_t *dest, size_t size, int64_t timeout)
{
  bool success = true;
  size_t pos = 0;
  uint64_t startTime = getTimeMS();
  int64_t remainingTime;

  while(success && (pos < size) && ((remainingTime = timeout - (getTimeMS() - startTime)) >= 0))
  {
    struct pollfd pollDesc;
    pollDesc.fd = sock;
    pollDesc.events = POLLIN;

    int error = poll(&pollDesc, 1, remainingTime);
    if(error <= 0)
    {
      success = false;
    }
    else
    {
      ssize_t numRead = read(sock, dest + pos, size - pos);
      if(numRead <= 0)
      {
        success = false;
      }
      else
      {
        pos += numRead;
      }
    }
  }

  return success;
}

bool BluetoothHCI::waitClose(int sock, int64_t timeout)
{
  bool success = true;
  uint64_t startTime = getTimeMS();

  struct pollfd pollDesc;
  pollDesc.fd = sock;
  pollDesc.events = POLLIN | POLLHUP;

  int error = poll(&pollDesc, 1, timeout);
  if(error <= 0)
  {
    success = false;
  }

  return success;
}
