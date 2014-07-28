#include "Address.h"

#include <cstring>
#include <cstdlib>
#include <string>

using namespace std;

Address::Address()
   : value_()
{
}

Address::Address(size_t size, const uint8_t *value)
   : value_(size, 0)
{
  if(value != NULL)
  {
    memcpy(value_.data(), value, size);
  }
}

Address Address::generateWithPartial(size_t size, uint8_t value, uint8_t mask)
{
  Address address(size);

  for(size_t i = 0; i < size; i++) 
  {
    address.value_[i] = rand() & 0xFF;
  }

  address.value_[0] = (address.value_[0] & ~mask) | (value & mask);
  address.value_[(size / 2) - 1] &= 0xF0;
  address.value_[(size / 2) - 1] |= computeChecksum(&address.value_[0], size / 2);
  address.value_[size - 1] &= 0xF0;
  address.value_[size - 1] |= computeChecksum(&address.value_[size / 2], size / 2); 

  return address;
}

Address Address::shiftWithPartial(uint8_t value, uint8_t mask) const
{
  Address address(value_.size());

  size_t halfSize = value_.size() / 2;
  for(size_t i = 0; i < halfSize; i++) 
  {
    address.value_[i + halfSize] = value_[i];
  }

  do
  {
    for(size_t i = 0; i < halfSize; i++) 
    {
      address.value_[i] = rand() & 0xFF; 
    }

    address.value_[0] = (address.value_[0] & ~mask) | (value & mask);
    address.value_[halfSize - 1] &= 0xF0; 
    address.value_[halfSize - 1] |= computeChecksum(&address.value_[0], value_.size() / 2); 
  }
  while(address == *this);

  return address;
}

Address Address::unshift() const
{
  Address address(value_.size());

  size_t halfSize = value_.size() / 2;
  for(size_t i = 0; i < halfSize; i++)
  {
    address.value_[i] = value_[i + halfSize];
  }

  return address;
}

Address Address::swap() const
{
  Address address(value_.size());

  for(size_t i = 0; i < value_.size(); i++) 
  {
    address.value_[i] = value_[value_.size() - i - 1];
  }

  return address;
}

string Address::toString() const
{
  static const char hexTable[] = "0123456789ABCDEF";

  string str((value_.size() * 3) - 1, ':');
  size_t i;
  for(i = 0; i < value_.size(); i++)
  {
    str[3 * i] = hexTable[value_[i] >> 4];
    str[(3 * i) + 1] = hexTable[value_[i] & 0x0F];
  }

  return str;
}

bool Address::isShift(const Address& other) const
{
  bool success = true;
  size_t halfSize = value_.size() / 2; 
  for(size_t i = 0; i < halfSize; i++) 
  {
    if(value_[i + halfSize] != other.value_[i]) 
    {
      success = false; 
    }
  }

  return success;
}

bool Address::verifyChecksum() const
{
  bool success = false;

  size_t halfSize = value_.size() / 2; 
  if((value_[halfSize - 1] & 0x0F) == computeChecksum(&value_[0], halfSize) &&
     (value_[value_.size() - 1] & 0x0F) == computeChecksum(&value_[halfSize], halfSize)) 
  {
    return true;
  }

  return success;
}

uint8_t Address::computeChecksum(const uint8_t *part, size_t size)
{
  uint8_t checksum = 0;

  size_t i;
  for(i = 0; i < size - 1; i++) 
  {
    checksum += (part[i] >> 4) + (part[i] & 0x0F); 
  }
  checksum += (part[i] >> 4); 

  return checksum & 0x0F;
}

