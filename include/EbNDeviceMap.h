#ifndef EBNDEVICEMAP_H
#define EBNDEVICEMAP_H

#include <type_traits>
#include <unordered_map>

#include "Address.h"
#include "EbNDevice.h"

// Maps addresses and device IDs to particular devices. Supports both exact and
// shifted matching of addresses, where shifted addresses are witnessed across
// epoch boundaries.
template<typename TDevice>
class EbNDeviceMap
{
  static_assert(std::is_base_of<EbNDevice, TDevice>::value, "TDevice must be derived from EbNDevice");

private:
  typedef typename std::unordered_map<Address, TDevice *, Address::Hash, Address::Equal> AddressToDeviceMap;
  typedef typename AddressToDeviceMap::iterator AddressToDeviceMap_Iter;
  typedef typename AddressToDeviceMap::const_iterator AddressToDeviceMap_ConstIter;
  typedef typename AddressToDeviceMap::const_local_iterator AddressToDeviceMap_ConstLocalIter;

  typedef typename std::unordered_map<DeviceID, TDevice *> DeviceIDToDeviceMap;
  typedef typename DeviceIDToDeviceMap::iterator DeviceIDToDeviceMap_Iter;
  typedef typename DeviceIDToDeviceMap::const_iterator DeviceIDToDeviceMap_ConstIter;

  AddressToDeviceMap addressToDevice_;
  DeviceIDToDeviceMap idToDevice_;

public:
  EbNDeviceMap();
  ~EbNDeviceMap();

  TDevice* findExactMatch(const Address& address) const;
  TDevice* findShiftedMatch(const Address& address);

  TDevice* get(DeviceID id);
  TDevice* get(const Address& address);
  void add(const Address& address, TDevice *device);
  bool remove(const Address& address);
  bool remove(DeviceID id);

  void clear();

  DeviceIDToDeviceMap_Iter begin();
  DeviceIDToDeviceMap_Iter end();
};

template<typename TDevice>
EbNDeviceMap<TDevice>::EbNDeviceMap()
{
}

template<typename TDevice>
EbNDeviceMap<TDevice>::~EbNDeviceMap()
{
  for(DeviceIDToDeviceMap_Iter it = idToDevice_.begin(); it != idToDevice_.end(); it++)
  {
    delete it->second;
  }
}

template<typename TDevice>
TDevice* EbNDeviceMap<TDevice>::findExactMatch(const Address &address) const
{
  AddressToDeviceMap_ConstIter it = addressToDevice_.find(address);
  if(it != addressToDevice_.end())
  {
    return it->second;
  }

  return NULL;
}

template<typename TDevice>
TDevice* EbNDeviceMap<TDevice>::findShiftedMatch(const Address &address)
{
  Address oldAddress = address.unshift();

  size_t bucket = addressToDevice_.bucket(oldAddress);
  for(AddressToDeviceMap_ConstLocalIter it = addressToDevice_.begin(bucket); it != addressToDevice_.end(bucket); it++)
  {
    if(address.isShift(it->first))
    {
      TDevice *device = it->second;
      device->setAddress(address);

      addressToDevice_.erase(it->first);
      addressToDevice_.insert(std::make_pair(address, device));

      return device;
    }
  }

  return NULL;
}

template<typename TDevice>
TDevice* EbNDeviceMap<TDevice>::get(DeviceID id)
{
  DeviceIDToDeviceMap_Iter it = idToDevice_.find(id);
  if(it != idToDevice_.end())
  {
    return it->second;
  }

  return NULL;
}

template<typename TDevice>
TDevice* EbNDeviceMap<TDevice>::get(const Address &address)
{
  TDevice *device = NULL;
  if((device = findExactMatch(address)) == NULL)
  {
    device = findShiftedMatch(address);
  }

  return device;
}

template<typename TDevice>
void EbNDeviceMap<TDevice>::add(const Address &address, TDevice *device)
{
  addressToDevice_.insert(std::make_pair(address, device));
  idToDevice_.insert(std::make_pair(device->getID(), device));
}

template<typename TDevice>
bool EbNDeviceMap<TDevice>::remove(const Address &address)
{
  AddressToDeviceMap_Iter it = addressToDevice_.find(address);
  if(it != addressToDevice_.end())
  {
    TDevice *device = it->second;

    idToDevice_.erase(idToDevice_.find(device->getID()));
    addressToDevice_.erase(it);
    delete device;

    return true;
  }

  return false;
}

template<typename TDevice>
bool EbNDeviceMap<TDevice>::remove(DeviceID id)
{
  DeviceIDToDeviceMap_Iter it = idToDevice_.find(id);
  if(it != idToDevice_.end())
  {
    TDevice *device = it->second;

    addressToDevice_.erase(addressToDevice_.find(device->getAddress()));
    idToDevice_.erase(it);
    delete device;

    return true;
  }

  return false;
}

template<typename TDevice>
void EbNDeviceMap<TDevice>::clear()
{
  addressToDevice_.clear();
  idToDevice_.clear();
}

template<typename TDevice>
typename EbNDeviceMap<TDevice>::DeviceIDToDeviceMap_Iter EbNDeviceMap<TDevice>::begin()
{
  return idToDevice_.begin();
}

template<typename TDevice>
typename EbNDeviceMap<TDevice>::DeviceIDToDeviceMap_Iter EbNDeviceMap<TDevice>::end()
{
  return idToDevice_.end();
}

#endif // EBNDEVICEMAP_H
