#ifndef SIPHASH_H
#define SIPHASH_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

class SipHash
{
private:
  std::vector<uint8_t> key_;

public:
  SipHash();
  SipHash(const uint8_t *key);

  uint64_t digest(const uint8_t *data, size_t size) const;
};

inline const SipHash& GSipHash()
{
  static SipHash instance;
  return instance;
}

#endif
