#ifndef ADDRESS_H
#define	ADDRESS_H

#include <cstdint>
#include <string>
#include <vector>

#include "SipHash.h"

class Address
{
public:
  struct Hash
  {
    // Uses only the top half of the address so that we can perform a bucket
    // lookup for shifted matches
    size_t operator()(const Address &address) const
    {
      return GSipHash().digest(address.value_.data(), address.value_.size() / 2);
    }
  };

  struct Equal
  {
    bool operator ()(const Address &a, const Address &b) const
    {
      return (a.value_ == b.value_);
    }
  };

private:
  std::vector<uint8_t> value_;

public:
  Address();
  Address(size_t size, const uint8_t *value = NULL);

  static Address generate(size_t size);
  static Address generateWithPartial(size_t size, uint8_t value, uint8_t mask);

  Address shift() const;
  Address shiftWithPartial(uint8_t value, uint8_t mask) const;
  Address unshift() const;

  Address swap() const;

  uint8_t getPartialValue(uint8_t mask) const;

  uint8_t* toByteArray(); 
  const uint8_t* toByteArray() const; 
  std::string toString() const; 

  bool operator==(const Address &other) const;
  bool operator!=(const Address &other) const; 
  bool isShift(const Address &other) const; 

  bool verifyChecksum() const;

private:
  static uint8_t computeChecksum(const uint8_t *part, size_t size);
};

inline Address Address::generate(size_t size)
{
  return generateWithPartial(size, 0, 0);
}

inline Address Address::shift() const
{
  return shiftWithPartial(0, 0);
}

inline uint8_t Address::getPartialValue(uint8_t mask) const
{
  return (value_[0] & mask);
}

inline uint8_t* Address::toByteArray()
{
  return value_.data();
}

inline const uint8_t* Address::toByteArray() const
{
  return value_.data();
}

inline bool Address::operator==(const Address &other) const
{
  return (value_ == other.value_);
}

inline bool Address::operator!=(const Address &other) const
{
  return (value_ != other.value_); 
}

#endif // ADDRESS_H

