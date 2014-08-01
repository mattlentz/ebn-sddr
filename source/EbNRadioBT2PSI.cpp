#include "EbNRadioBT2PSI.h"

#include "AndroidWake.h"

#include <future>

using namespace std;

EbNRadio::ConfirmScheme EbNRadioBT2PSI::getDefaultConfirmScheme()
{
  return {ConfirmScheme::Active, 0};
}

EbNRadioBT2PSI::EbNRadioBT2PSI(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID)
   : EbNRadio(keySize, confirmScheme, memoryScheme),
     BF_M((238 * 8) - 1 - ADV_N_LOG2 - keySize),
     BF_K(4),
     hci_(adapterID),
     deviceMap_(),
     dhExchange_(keySize),
     advertNum_(0),
     listenThread_()
{
  if(confirmScheme.type != ConfirmScheme::Active)
  {
    throw std::runtime_error("Invalid Confirmation Scheme for EbNRadioBT2PSI: Only supports 'Active'.");
  }
}

void EbNRadioBT2PSI::setAdvertisedSet(const LinkValueList &advertisedSet)
{
  lock_guard<mutex> setLock(setMutex_);
  advertisedSet_ = advertisedSet;

  psiServerData_.clear();
  for(auto it = advertisedSet.cbegin(); it != advertisedSet.cend(); it++)
  {
    psiServerData_.push_back(string((char *)it->get(), dhExchange_.getKeySize() / 8));
  }
}

void EbNRadioBT2PSI::setListenSet(const LinkValueList &listenSet)
{
  lock_guard<mutex> setLock(setMutex_);
  listenSet_ = listenSet;

  psiClientData_.clear();
  for(auto it = listenSet.cbegin(); it != listenSet.cend(); it++)
  {
    psiClientData_.push_back(string((char *)it->get(), dhExchange_.getKeySize() / 8));
  }
}

void EbNRadioBT2PSI::initialize()
{
  // Disabling discoverable and EIR settings so that we can set up the new
  // address and first payload before remote devices can receive it
  hci_.setDiscoverable(false);

  hci_.setPublicAddress(Address::generate(6));

  hci_.setInquiryMode(BluetoothHCI::InquiryMode::WithRSSIAndEIR);
  hci_.setDiscoverable(true);
  hci_.setConnectable(true);

  // Start a thread to listen for incoming connections in the case of active or
  // hybrid confirmation schemes
  listenThread_ = thread(&EbNRadioBT2PSI::listen, this);

  Memreuse::Initialize(4096, 1024);
}

list<DiscoverEvent> EbNRadioBT2PSI::discover()
{
  if(memoryScheme_ == MemoryScheme::NoMemory)
  {
    deviceMap_.clear();
  }

  unique_lock<mutex> discDevicesLock(discDevicesMutex_);
  discDevices_.clear();
  discDevicesLock.unlock();

  list<DiscoverEvent> discovered;
  function<void(const EIRInquiryResponse *)> callback = bind(&EbNRadioBT2PSI::processEIRResponse, this, &discovered, placeholders::_1);
  hci_.performEIRInquiry(callback, DISC_PERIODS);

  nextDiscover_ += DISC_INTERVAL - 1000 + (rand() % 2001);

  return discovered;
}

void EbNRadioBT2PSI::changeEpoch()
{
  advertNum_ = 0;

  // Generate a new secret for this epoch's DH exchanges
  dhExchange_.generateSecret();

  // Shifting the current address. Uses the 1 bit Y coordinate as part of the
  // address since compressed ECDH keys are actually (keySize + 1) bits long
  hci_.setPublicAddress(hci_.getPublicAddress().shift());

  hci_.setDiscoverable(false);
  hci_.setInquiryMode(BluetoothHCI::InquiryMode::WithRSSIAndEIR);
  hci_.setDiscoverable(true);

  nextChangeEpoch_ += EPOCH_INTERVAL;
}

set<DeviceID> EbNRadioBT2PSI::handshake(const set<DeviceID> &deviceIDs)
{
  set<DeviceID> encountered;

  for(auto it = deviceIDs.begin(); it != deviceIDs.end(); it++)
  {
    EbNDeviceBT2 *device = deviceMap_.get(*it);

    // Only perform active confirmation once per device
    if(!device->isConfirmed() && (getHandshakeScheme() == ConfirmScheme::Active))
    {
      // TODO: Make sure that there is enough time to contact the remote
      // device (or time out) prior to the next action

      LOG_D("EbNRadioBT2PSI", "Attempting to connect to device (ID %d, Address %s)", device->getID(), device->getAddress().toString().c_str());

      unique_lock<mutex> discDevicesLock(discDevicesMutex_);
      if(discDevices_.insert(device->getAddress()).second == false)
      {
        LOG_D("EbNRadioBT2PSI", "Skipping connection, already performed during this discovery period");
        break;
      }
      discDevicesLock.unlock();

      int sock = hci_.connectBT2(device->getAddress(), 1, 5000);
      if(sock >= 0)
      {
        LOG_D("EbNRadioBT2PSI", "Connected!");

        Client *psiClient = new JL10_Client();
        Server *psiServer = new JL10_Server();
        psiClient->SetAdaptive();
        psiServer->SetAdaptive();
        psiClient->Setup(1024);
        psiServer->Setup(1024);

        unique_lock<mutex> setLock(setMutex_);
        psiClient->LoadData(psiClientData_);
        psiServer->LoadData(psiServerData_);
        setLock.unlock();

        psiClient->Initialize(1024);
        psiServer->Initialize(1024);

        LOG_D("EbNRadioBT2PSI", "PSI Server and Client initialized...");

        uint32_t messageSize = 0;
        vector<uint8_t> message;
        vector<string> output;
        bool success = true;

        //Exchanging and computing the shared secret
        if(success && (success = hci_.send(sock, dhExchange_.getPublic(), dhExchange_.getPublicSize(), 30000)))
        {
          vector<uint8_t> remotePublic(dhExchange_.getPublicSize());
          if(success && (success = hci_.recv(sock, remotePublic.data(), dhExchange_.getPublicSize(), 30000)))
          {
            SharedSecret sharedSecret(SharedSecret::ConfirmScheme::Active);
            if(dhExchange_.computeSharedSecret(sharedSecret, remotePublic.data()))
            {
              LOG_E("EbNRadioBT2PSI", "Computed shared secret!");
              device->addSharedSecret(sharedSecret);
            }
            else
            {
              LOG_E("EbNRadioBT2PSI", "Could not compute shared secret");
            }
          }
        }

        //Act as client first
        if(success && (success = hci_.recv(sock, (uint8_t *)&messageSize, 4, 30000)))
        {
          LOG_D("EbNRadioBT2PSI", "HC1 - Incoming message of size %u", messageSize);
          message.resize(messageSize);

          if(success && (success = hci_.recv(sock, message.data(), messageSize, 30000)))
          {
            vector<string> publishedData = parsePSIMessage(message);
            psiClient->StoreData(publishedData);

            output.clear();
            (psiClient->*(psiClient->m_vecRequest[0]))(output);

            constructPSIMessage(message, output);
            LOG_D("EbNRadioBT2PSI", "HC1 - Sending message of size %zu", message.size());
            success = hci_.send(sock, message.data(), message.size(), 30000);
          }
        }

        if(success && (success = hci_.recv(sock, (uint8_t *)&messageSize, 4, 30000)))
        {
          LOG_D("EbNRadioBT2PSI", "HC2 - Incoming message of size %u", messageSize);
          message.resize(messageSize);

          if(success && (success = hci_.recv(sock, message.data(), messageSize, 30000)))
          {
            vector<string> input = parsePSIMessage(message);
            (psiClient->*(psiClient->m_vecOnResponse[0]))(input);

            LOG_D("EbNRadioBT2PSI", "HC2 - Result (Size = %zu)", psiClient->m_vecResult.size());
          }
        }

        //Act as server second
        if(success)
        {
          output.clear();
          psiServer->PublishData(output);
          constructPSIMessage(message, output);
          LOG_D("EbNRadioBT2PSI", "HS1 - Sending message of size %zu", message.size());
          success = hci_.send(sock, message.data(), message.size(), 30000);
        }

        if(success && (success = hci_.recv(sock, (uint8_t *)&messageSize, 4, 30000)))
        {
          LOG_D("EbNRadioBT2PSI", "HS2 - Incoming message of size %u", messageSize);
          message.resize(messageSize);

          if(success && (success = hci_.recv(sock, message.data(), messageSize, 30000)))
          {
            vector<string> input = parsePSIMessage(message);
            (psiServer->*(psiServer->m_vecOnRequest[0]))(input);

            output.clear();
            (psiServer->*(psiServer->m_vecResponse[0]))(output);

            constructPSIMessage(message, output);
            LOG_D("EbNRadioBT2PSI", "HS2 - Sending message of size %zu", message.size());
            success = hci_.send(sock, message.data(), message.size(), 30000);
          }
        }

        // TODO: Process the result to update matching set

        delete psiClient;
        delete psiServer;

        LOG_D("EbNRadioBT2PSI", "Waiting for remote client to close connection...");
        hci_.waitClose(sock, 30000);
        LOG_D("EbNRadioBT2PSI", "Done!");

        close(sock);
      }
    }

    device->setShakenHands(true);
  }

  // Going through all devices to report 'encountered' devices, meaning
  // the devices we have shaken hands with and confirmed
  for(auto it = deviceMap_.begin(); it != deviceMap_.end(); it++)
  {
    EbNDevice *device = it->second;
    if(device->hasShakenHands() && device->isConfirmed())
    {
      encountered.insert(device->getID());
    }
  }

  return encountered;
}

EncounterEvent EbNRadioBT2PSI::doneWithDevice(DeviceID id)
{
  EbNDeviceBT2 *device = deviceMap_.get(id);

  EncounterEvent expiredEvent(getTimeMS());
  device->getEncounterInfo(expiredEvent, true);

  deviceMap_.remove(id);
  removeRecentDevice(id);

  return expiredEvent;
}

void EbNRadioBT2PSI::processEIRResponse(list<DiscoverEvent> *discovered, const EIRInquiryResponse *resp)
{
  uint64_t scanTime = getTimeMS();

  bool addressOK = resp->address.verifyChecksum();
  if(addressOK)
  {
    EbNDeviceBT2 *device = deviceMap_.get(resp->address);
    if(device == NULL)
    {
      lock_guard<mutex> setLock(setMutex_);

      device = new EbNDeviceBT2(generateDeviceID(), resp->address, resp->clockOffset, resp->pageScanMode, listenSet_);
      deviceMap_.add(resp->address, device);

      LOG_P("EbNRadioBT2PSI", "Discovered new EbN device (ID %d, Address %s)", device->getID(), device->getAddress().toString().c_str());
    }

    device->addRSSIMeasurement(scanTime, resp->rssi);
    discovered->push_back(DiscoverEvent(scanTime, device->getID(), resp->rssi));
    addRecentDevice(device);
  }
  else
  {
    LOG_D("EbNRadioBT2PSI", "Discovered non-EbN device (Address %s) [Checksum OK? %d]", resp->address.toString().c_str(), addressOK);
  }
}

void EbNRadioBT2PSI::listen()
{
  while(true)
  {
    LOG_D("EbNRadioBT2PSI", "Attempting to listen for incoming BT2 connections");

    int listenSock = hci_.listenBT2(1);

    LOG_D("EbNRadioBT2PSI", "Waiting to accept incoming BT2 connection");

    pair<int, Address> clientInfo;
    while((clientInfo = hci_.acceptBT2(listenSock)).first != -1)
    {
      AndroidWake::grab("EbNListen");

      int sock = clientInfo.first;
      Address clientAddress = clientInfo.second;
      LOG_D("EbNRadioBT2PSI", "Accepted incoming connection from %s", clientAddress.toString().c_str());

      unique_lock<mutex> discDevicesLock(discDevicesMutex_);
      discDevices_.insert(clientAddress);
      discDevicesLock.unlock();

      Client *psiClient = new JL10_Client();
      Server *psiServer = new JL10_Server();
      psiClient->SetAdaptive();
      psiServer->SetAdaptive();
      psiClient->Setup(1024);
      psiServer->Setup(1024);

      unique_lock<mutex> setLock(setMutex_);
      psiClient->LoadData(psiClientData_);
      psiServer->LoadData(psiServerData_);
      setLock.unlock();

      psiClient->Initialize(1024);
      psiServer->Initialize(1024);

      LOG_D("EbNRadioBT2PSI", "PSI Server and Client initialized...");

      uint32_t messageSize = 0;
      vector<uint8_t> message;
      vector<string> output;
      bool success = true;

      //Exchanging and computing the shared secret
      if(success && (success = hci_.send(sock, dhExchange_.getPublic(), dhExchange_.getPublicSize(), 30000)))
      {
        vector<uint8_t> remotePublic(dhExchange_.getPublicSize());
        if(success && (success = hci_.recv(sock, remotePublic.data(), dhExchange_.getPublicSize(), 30000)))
        {
          SharedSecret sharedSecret(SharedSecret::ConfirmScheme::Active);
          if(dhExchange_.computeSharedSecret(sharedSecret, remotePublic.data()))
          {
            LOG_E("EbNRadioBT2PSI", "Computed shared secret!");
            // TODO: Add shared secret for the device
          }
          else
          {
            LOG_E("EbNRadioBT2PSI", "Could not compute shared secret");
          }
        }
      }

      //PSI: Act as server first
      if(success)
      {
        output.clear();
        psiServer->PublishData(output);
        constructPSIMessage(message, output);
        LOG_D("EbNRadioBT2PSI", "LS1 - Sending message of size %zu", message.size());
        success = hci_.send(sock, message.data(), message.size(), 30000);
      }

      if(success && (success = hci_.recv(sock, (uint8_t *)&messageSize, 4, 30000)))
      {
        LOG_D("EbNRadioBT2PSI", "LS2 - Incoming message of size %u", messageSize);
        message.resize(messageSize);

        if(success && (success = hci_.recv(sock, message.data(), messageSize, 30000)))
        {
          vector<string> input = parsePSIMessage(message);
          (psiServer->*(psiServer->m_vecOnRequest[0]))(input);

          output.clear();
          (psiServer->*(psiServer->m_vecResponse[0]))(output);

          constructPSIMessage(message, output);
          LOG_D("EbNRadioBT2PSI", "LS2 - Sending message of size %zu", message.size());
          success = hci_.send(sock, message.data(), message.size(), 30000);
        }
      }

      //PSI: Act as client second
      if(success && (success = hci_.recv(sock, (uint8_t *)&messageSize, 4, 30000)))
      {
        LOG_D("EbNRadioBT2PSI", "LC1 - Incoming message of size %u", messageSize);
        message.resize(messageSize);

        if(success && (success = hci_.recv(sock, message.data(), messageSize, 30000)))
        {
          vector<string> publishedData = parsePSIMessage(message);
          psiClient->StoreData(publishedData);

          output.clear();
          (psiClient->*(psiClient->m_vecRequest[0]))(output);

          constructPSIMessage(message, output);
          LOG_D("EbNRadioBT2PSI", "LC1 - Sending message of size %zu", message.size());
          success = hci_.send(sock, message.data(), message.size(), 30000);
        }
      }

      if(success && (success = hci_.recv(sock, (uint8_t *)&messageSize, 4, 30000)))
      {
        LOG_D("EbNRadioBT2PSI", "LC2 - Incoming message of size %u", messageSize);
        message.resize(messageSize);

        if(success && (success = hci_.recv(sock, message.data(), messageSize, 30000)))
        {
          vector<string> input = parsePSIMessage(message);
          (psiClient->*(psiClient->m_vecOnResponse[0]))(input);

          LOG_D("EbNRadioBT2PSI", "LC2 - Result (Size = %zu)", psiClient->m_vecResult.size());
        }
      }

      // TODO: Process the result to update matching set

      close(sock);

      AndroidWake::release("EbNListen");
    }

    close(listenSock);

    LOG_D("EbNRadioBT2PSI", "Restarting the listen thread after 5 seconds...");
    sleepSec(5);
  }
}

void EbNRadioBT2PSI::constructPSIMessage(vector<uint8_t> &message, vector<string> input)
{
  size_t pos = 0;

  uint32_t messageSize = BasicTool::StringVectorSize(input) + (input.size() * 4);
  message.resize(4 + messageSize);

  memcpy(message.data() + pos, &messageSize, 4);
  pos += 4;

  for(auto it = input.begin(); it != input.end(); it++)
  {
    uint32_t elemLength = it->length();
    memcpy(message.data() + pos, &elemLength, 4);
    pos += 4;

    memcpy(message.data() + pos, it->c_str(), elemLength);
    pos += elemLength;
  }
}

vector<string> EbNRadioBT2PSI::parsePSIMessage(const vector<uint8_t> &message)
{
  size_t pos = 0;
  vector<string> output;

  while(pos < message.size())
  {
    uint32_t elemLength;
    memcpy(&elemLength, message.data() + pos, 4);
    pos += 4;

    output.push_back(string((const char *)message.data() + pos, elemLength));
    pos += elemLength;
  }

  return output;
}
