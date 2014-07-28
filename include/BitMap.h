#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class BitMap
{
private:
  size_t size_;
  std::vector<uint8_t> bits_;

public:
  BitMap();
  BitMap(size_t size, const uint8_t *bits = NULL);

  void set(size_t index);
  void set(size_t index, bool value); 
  void setByte(size_t index, uint8_t value); 
  void setAll(bool value);
  bool get(size_t index) const;
  bool getThenSet(size_t index);

  size_t size() const;
  size_t sizeBytes() const;

  uint8_t* toByteArray();
  const uint8_t* toByteArray() const;

  void copyFrom(const uint8_t *source, size_t sourceOffset, size_t offset, size_t length);
  void copyTo(uint8_t *dest, size_t destOffset, size_t offset, size_t length) const;

  std::string toString() const;
  std::string toHexString() const;

private:
  void bitcpy(uint8_t *dest, size_t destOffset, const uint8_t *source, size_t sourceOffset, size_t length) const;
};

inline void BitMap::set(size_t index)
{
  bits_[index >> 3] |= (1 << (index & 0x7));
}

inline void BitMap::set(size_t index, bool value)
{
  bits_[index >> 3] = (bits_[index >> 3] & ~(1 << (index & 0x7))) | (value << (index & 0x7)); 
}

inline void BitMap::setByte(size_t index, uint8_t value)
{
  bits_[index] = value;
}

inline void BitMap::setAll(bool value)
{
  memset(bits_.data(), value ? 0xFF : 0x00, (size_ + 7) / 8);
}

inline bool BitMap::get(size_t index) const
{
  return (bits_[index >> 3] & (1 << (index & 0x7))) != 0;
}

inline bool BitMap::getThenSet(size_t index)
{
  bool value = get(index);
  set(index);
  return value;
}

inline size_t BitMap::size() const
{
  return size_;
}

inline size_t BitMap::sizeBytes() const
{
  return (size_ + 7) / 8;
}

inline uint8_t* BitMap::toByteArray()
{
  return bits_.data();
}

inline const uint8_t* BitMap::toByteArray() const
{
  return bits_.data();
}

inline void BitMap::copyFrom(const uint8_t *source, size_t sourceOffset, size_t offset, size_t length)
{
  bitcpy(bits_.data(), offset, source, sourceOffset, length); 
}

inline void BitMap::copyTo(uint8_t *dest, size_t destOffset, size_t offset, size_t length) const
{
  bitcpy(dest, destOffset, bits_.data(), offset, length);
}

#endif  // BITMAP_H

