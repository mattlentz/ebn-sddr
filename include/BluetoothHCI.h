#ifndef BLUETOOTHHCI_H
#define	BLUETOOTHHCI_H

#include <cstdint>
#include <functional>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>

#include "bluetooth/bluetooth.h"
#include "bluetooth/hci.h"

#include "Address.h"

// For exceptional errors caused by interactions with the Bluetooth controller
// through the host-controller interface, which we cannot recover from
class BluetoothHCIException : public std::runtime_error
{
public:
  BluetoothHCIException() : std::runtime_error("BluetoothHCIException") { }
  BluetoothHCIException(int error) : std::runtime_error(std::string("BluetoothHCIException - ") + strerror(error)) { }
  BluetoothHCIException(std::string message) : std::runtime_error(std::string("BluetoothHCIException - ") + message) { }
};

struct InquiryResponse
{
  Address address;
  uint16_t clockOffset;
  uint8_t pageScanMode;
  uint8_t pageScanPeriodMode;
};

struct EIRInquiryResponse : InquiryResponse
{
  uint8_t *data;
  int8_t rssi;
};

struct ScanResponse
{
  Address address;
  uint8_t *data;
  uint8_t length;
  int8_t rssi;
};

// Unfortunately the Bluetooth headers contained in the Android source do
// not support the LE functionality that we require. Therefore, the
// appropriate constants as contained in the specification are listed below.
class BluetoothHCI
{
public:
  struct AdvertFilter_
  {
    enum Type
    {
      ScanAllConnectAll = 0,
      ScanWhiteConnectAll = 1,
      ScanAllConnectWhite = 2,
      ScanWhiteConnectWhite = 3
    };
  };
  typedef AdvertFilter_::Type AdvertFilter;

  struct DirectedAdvert_
  {
    enum Type
    {
      Connectable = 1
    };
  };
  typedef DirectedAdvert_::Type DirectedAdvert;

  struct UndirectedAdvert_
  {
    enum Type
    {
      Connectable = 0,
      Scannable = 2,
      Standard = 3
    };
  };
  typedef UndirectedAdvert_::Type UndirectedAdvert;

  struct ChannelMap_
  {
    enum Type
    {
      First = 1,
      Second = 2,
      Third = 4,
      All = 7
    };
  };
  typedef ChannelMap_::Type ChannelMap;

  struct InquiryMode_
  {
    enum Type
    {
      Standard = 0,
      WithRSSI = 1,
      WithRSSIAndEIR = 2,
    };
  };
  typedef InquiryMode_::Type InquiryMode;

  struct ScanFilter_
  {
    enum Type
    {
      All = 0,
      White = 1
    };
  };
  typedef ScanFilter_::Type ScanFilter;

  struct DuplicateFilter_
  {
    enum Type
    {
      Off = 0,
      On = 1
    };
  };
  typedef DuplicateFilter_::Type DuplicateFilter;

  struct Scan_
  {
    enum Type
    {
      Passive = 0,
      Active = 1
    };
  };
  typedef Scan_::Type Scan;

private:
  struct LastKnownState
  {
    bool publicAddressStored;
    bool randomAddressStored;
    bool isDiscoverableStored;
    bool isConnectableStored;
    bool inquiryModeStored;
    bool localNameStored;
    bool isAdvertisingStored;
    bool advertDataStored;
    bool responseDataStored;
    bool advertParamsStored;

    Address publicAddress;
    Address randomAddress;
    bool isDiscoverable;
    bool isConnectable;
    uint8_t inquiryMode;
    std::string localName;
    bool isAdvertising;
    le_set_advertising_data_cp advertData;
    le_set_scan_response_data_cp responseData;
    le_set_advertising_parameters_cp advertParams;
  };

private:
  int adapterID_;
  int sock_;

  // Holds state (e.g. EIR payload) that gets overwritten after the Bluetooth
  // controller is reset. In the event that we need to recover the adapter due
  // to a fatal error, we will attempt to reinitialize the adapter based on this
  // state information.
  LastKnownState lastKnownState_;

public:
  BluetoothHCI(int adapterID);
  ~BluetoothHCI();

  int getAdapterID() const;

  void readState();

  void setConnectable(bool value);
  void setDiscoverable(bool value);
  void setInquiryMode(InquiryMode value);

  Address getPublicAddress();
  Address getRandomAddress();
  void setPublicAddress(const Address &address);
  void setRandomAddress(const Address &address);

  std::string readLocalName();
  bool readRemoteName(std::string &name, const Address &address, uint16_t clockOffset, uint8_t pageScanMode, uint64_t timeout);
  void writeLocalName(std::string name);

  void writeExtInquiryResponse(uint8_t *data);

  std::list<InquiryResponse> performInquiry(int periods = 8);
  void performEIRInquiry(std::function<void(const EIRInquiryResponse *)> callback, int periods = 8);
  void performScan(Scan type, DuplicateFilter filter, uint64_t duration, std::function<void(const ScanResponse *)> callback);

  void setAdvertData(const uint8_t *data, uint8_t length);
  void setScanResponseData(const uint8_t *data, uint8_t length);
  void setUndirectedAdvertParams(UndirectedAdvert type, AdvertFilter filter, uint16_t minInterval, uint16_t maxInterval);
  void enableAdvertising(bool enable);

private:
  void setScan(bool pscan, bool iscan);
  void recover();

public:
  // TODO: Should move this functionality into other classes
  static int connectBT2(const Address &address, uint8_t port, int64_t timeout);
  static int connectBT4(const Address &address, int64_t timeout);
  static int listenBT2(uint8_t port);
  static int listenBT4();
  static std::pair<int, Address> acceptBT(int sockListen);
  static bool send(int sock, const uint8_t *message, size_t size, int64_t timeout);
  static bool recv(int sock, uint8_t *dest, size_t size, int64_t timeout);
  static bool waitClose(int sock, int64_t timeout);
};

inline int BluetoothHCI::getAdapterID() const
{
  return adapterID_;
}

#endif  // BLUETOOTHHCI_H
