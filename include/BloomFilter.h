#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <cmath>
#include <cstdint>
#include <memory>
#include <openssl/bn.h>

#include "BitMap.h"

class BloomFilter
{
protected:
  size_t N_;
  size_t M_;
  size_t K_;
  BitMap bits_;
  float pFalse_;

public:
  BloomFilter();
  BloomFilter(size_t N, size_t M, size_t K);
  BloomFilter(size_t N, size_t M, size_t K, const uint8_t *bits);
  BloomFilter(size_t N, size_t K, const BitMap &bits);
  BloomFilter(size_t N, size_t K, const BitMap &bits, size_t offset, size_t length);
  virtual ~BloomFilter(); 

  size_t N() const;
  size_t M() const;
  size_t K() const;
  float pFalse() const;

  size_t sizeBytes() const;
  uint8_t* toByteArray(); 
  const uint8_t* toByteArray() const; 
  std::string toString() const;

  void set(size_t index);
  bool get(size_t index) const;

  std::unique_ptr<uint8_t[]> getHash(const uint8_t *value, size_t size) const;

  void add(const uint8_t *value, size_t size);
  void add(const uint8_t *prefix, size_t prefixSize, const uint8_t *value, size_t valueSize);
  void add(const uint8_t *hash);
  void addRandom(size_t count = 1);

  bool query(const uint8_t *value, size_t size) const;
  bool query(const uint8_t *prefix, size_t prefixSize, const uint8_t *value, size_t valueSize) const;
  bool query(const uint8_t *hash) const;

  void copyTo(uint8_t *dest, size_t destOffset) const;

public:
  void computeHash(uint8_t *dest, const uint8_t *value, size_t size) const;
  void computeHash(uint8_t *dest, const uint8_t *prefix, size_t prefixSize, const uint8_t *value, size_t valueSize) const;

public:
  static float computePFalse(size_t N, size_t M, size_t K);
};

inline size_t BloomFilter::N() const
{
  return N_;
}

inline size_t BloomFilter::M() const
{
  return M_;
}

inline size_t BloomFilter::K() const
{
  return K_;
}

inline float BloomFilter::pFalse() const
{
  return pFalse_;
}

inline size_t BloomFilter::sizeBytes() const
{
  return bits_.sizeBytes();
}

inline uint8_t* BloomFilter::toByteArray()
{
  return bits_.toByteArray();
}

inline const uint8_t* BloomFilter::toByteArray() const
{
  return bits_.toByteArray();
}

inline std::string BloomFilter::toString() const
{
  return bits_.toString();
}

inline void BloomFilter::set(size_t index)
{
  bits_.set(index);
}

inline bool BloomFilter::get(size_t index) const
{
  return bits_.get(index);
}

inline void BloomFilter::copyTo(uint8_t *dest, size_t offset) const
{
  bits_.copyTo(dest, offset, 0, bits_.size());
}

inline float BloomFilter::computePFalse(size_t N, size_t M, size_t K)
{
  return pow((1 - exp(-(float)K * N / M)), (float)K);
}

#endif  // BLOOMFILTER_H
