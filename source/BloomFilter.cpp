#include <cstring>
#include <endian.h>
#include <openssl/sha.h>

#include "BloomFilter.h"

using namespace std;

BloomFilter::BloomFilter()
   : N_(0),
     M_(0),
     K_(0),
     bits_(),
     pFalse_(1)
{
}

BloomFilter::BloomFilter(size_t N, size_t M, size_t K)
   : N_(N),
     M_(M),
     K_(K),
     bits_(M),
     pFalse_(computePFalse(N, M, K))
{
}

BloomFilter::BloomFilter(size_t N, size_t M, size_t K, const uint8_t *bits)
   : N_(N),
     M_(M),
     K_(K),
     bits_(M, bits),
     pFalse_(computePFalse(N, M, K)) 
{
}

BloomFilter::BloomFilter(size_t N, size_t K, const BitMap &bits)
   : N_(N),
     M_(bits.size()),
     K_(K),
     bits_(bits),
     pFalse_(computePFalse(N, M_, K)) 
{
}

BloomFilter::BloomFilter(size_t N, size_t K, const BitMap &bits, size_t offset, size_t length)
   : N_(N),
     M_(length),
     K_(K),
     bits_(length),
     pFalse_(computePFalse(N, M_, K)) 
{
  bits_.copyFrom(bits.toByteArray(), offset, 0, length);
}

BloomFilter::~BloomFilter()
{
}

unique_ptr<uint8_t[]> BloomFilter::getHash(const uint8_t *value, size_t size) const
{
  unique_ptr<uint8_t[]> hash(new uint8_t[SHA256_DIGEST_LENGTH]);
  computeHash(hash.get(), value, size);
  return hash;
}

void BloomFilter::add(const uint8_t *value, size_t size)
{
  uint8_t hash[SHA256_DIGEST_LENGTH];
  computeHash(hash, value, size);
  add(hash);
}

void BloomFilter::add(const uint8_t *prefix, uint32_t prefixSize, const uint8_t *value, uint32_t valueSize)
{
  uint8_t hash[SHA256_DIGEST_LENGTH];
  computeHash(hash, prefix, prefixSize, value, valueSize);
  add(hash);
}

void BloomFilter::add(const uint8_t *hash)
{
  // TODO: For our purposes, we will never end up using larger values of K and M
  // for which this method may produce less than adequate false positive rates.
  // This should eventually support the previous algorithm for those other
  // cases.
  for(size_t k = 0; k < K_; k++)
  {
    uint32_t index = htole32(((uint32_t *)hash)[k]);
    bits_.set(index % M_);
  }
}

void BloomFilter::addRandom(size_t count)
{
  for(size_t k = 0; k < K_ * count; k++)
  {
    set(rand() % M_);
  }
}

bool BloomFilter::query(const uint8_t *entity, uint32_t size) const
{
  uint8_t hash[SHA256_DIGEST_LENGTH];
  computeHash(hash, entity, size);
  return query(hash);
}

bool BloomFilter::query(const uint8_t *prefix, uint32_t prefixSize, const uint8_t *value, uint32_t valueSize) const
{
  uint8_t hash[SHA256_DIGEST_LENGTH];
  computeHash(hash, prefix, prefixSize, value, valueSize);
  return query(hash);
}

bool BloomFilter::query(const uint8_t *hash) const
{
  // TODO: For our purposes, we will never end up using larger values of K and M
  // for which this method may produce less than adequate false positive rates.
  // This should eventually support the previous algorithm for those other
  // cases.
  for(size_t k = 0; k < K_; k++)
  {
    uint32_t index = htole32(((uint32_t *)hash)[k]);
    if(!bits_.get(index % M_))
    {
      return false;
    }
  }

  return true;
}

void BloomFilter::computeHash(uint8_t *dest, const uint8_t *value, size_t size) const
{
  SHA256_CTX hashCtx;
  SHA256_Init(&hashCtx);
  SHA256_Update(&hashCtx, value, size);
  SHA256_Final(dest, &hashCtx);
}

void BloomFilter::computeHash(uint8_t *dest, const uint8_t *prefix, size_t prefixSize, const uint8_t *value, size_t valueSize) const
{
  SHA256_CTX hashCtx;
  SHA256_Init(&hashCtx);
  SHA256_Update(&hashCtx, prefix, prefixSize);
  SHA256_Update(&hashCtx, value, valueSize);
  SHA256_Final(dest, &hashCtx);
}

