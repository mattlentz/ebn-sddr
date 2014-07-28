#include "BitMap.h"

using namespace std;

BitMap::BitMap()
   : size_(0),
     bits_()
{
}

BitMap::BitMap(size_t size, const uint8_t *bits)
   : size_(size),
     bits_((size + 7) / 8, 0)
{
  if(bits != NULL)
  {
    memcpy(bits_.data(), bits, (size + 7) / 8);
  }
}

string BitMap::toString() const
{
  string str(size_, '0');
  for(int b = 0; b < size_; b++)
  {
    if(get(b))
    {
      str[b] = '1';
    }
  }
  return str;
}

string BitMap::toHexString() const
{
  static const char hexTable[] = "0123456789ABCDEF";

  string str(2 * ((size_ + 7) / 8), '0');
  for(int b = 0; b < ((size_ + 7) / 8); b++)
  {
    uint8_t value = bits_[b];
    str[2 * b] = hexTable[value >> 4];
    str[(2 * b) + 1] = hexTable[value & 0xF];
  }
  return str;
}

void BitMap::bitcpy(uint8_t *dest, size_t destOffset, const uint8_t *source, size_t sourceOffset, size_t length) const
{
  uint32_t sourcePos = sourceOffset / 8;
  uint32_t sourceBitOffset = sourceOffset % 8;

  uint32_t destPos = destOffset / 8;
  uint32_t destBitOffset = destOffset % 8;

  //Start - Syncing to byte alignment on destination
  if(destBitOffset != 0)
  {
    uint32_t amount = min(8 - destBitOffset, length);

    if(amount <= (8 - sourceBitOffset))
    {
      dest[destPos] ^= (((dest[destPos] >> destBitOffset) ^ (source[sourcePos] >> sourceBitOffset)) & ((1 << amount) - 1)) << destBitOffset;
    }
    else
    {
      dest[destPos] ^= (((dest[destPos] >> destBitOffset) ^ ((source[sourcePos] >> sourceBitOffset) | (source[sourcePos + 1] << (8 - sourceBitOffset)))) & ((1 << amount) - 1)) << destBitOffset;
    }

    sourcePos = (sourceOffset + amount) / 8;
    sourceBitOffset = (sourceOffset + amount) % 8;
    destPos++;
    destBitOffset = 0;
    length -= amount;
  }

  //Body - Copying full bytes to destination
  uint32_t fullLengthBytes = length / 8;
  for(int b = 0; b < fullLengthBytes; b++)
  {
    dest[destPos] = (source[sourcePos] >> sourceBitOffset) | (source[sourcePos + 1] << (8 - sourceBitOffset));
    sourcePos++; destPos++;
  }
  length = length % 8;

  //Ending - Any remaining bits
  if(length != 0)
  {
    if(length <= (8 - sourceBitOffset))
    {
      dest[destPos] ^= (dest[destPos] ^ (source[sourcePos] >> sourceBitOffset)) & ((1 << length) - 1);
    }
    else
    {
      dest[destPos] ^= (dest[destPos] ^ ((source[sourcePos] >> sourceBitOffset) | (source[sourcePos + 1] << (8 - sourceBitOffset)))) & ((1 << length) - 1);
    }
  }
}

