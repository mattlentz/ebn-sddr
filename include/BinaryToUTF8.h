#ifndef BINARYTOUTF8_H
#define BINARYTOUTF8_H

#include <cstdint>
#include <string>

class BinaryToUTF8
{
public:
  static void encode(char *output, const uint8_t *input, size_t length);
  static std::string encode(const uint8_t *input, size_t length);
  static void decode(uint8_t *output, const char *input, size_t length);
  static void decode(uint8_t *output, const std::string &input);

  static size_t getNumEncodedBits(size_t length); 
  static size_t getNumDecodedBits(size_t length); 
};

inline void BinaryToUTF8::decode(uint8_t *output, const std::string &input)
{
  decode(output, input.c_str(), input.size() * 8);
}

inline size_t BinaryToUTF8::getNumEncodedBits(size_t length)
{
  return ((length + 6) / 7) * 8;
}

inline size_t BinaryToUTF8::getNumDecodedBits(size_t length)
{
  return (length / 8) * 7;
}

#endif  // BINARYTOUTF8_H

