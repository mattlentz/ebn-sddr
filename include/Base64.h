#ifndef BASE64_H
#define BASE64_H

#include <cstdint>
#include <string>

class Base64
{
private:
  static char encodeTable[];
  static uint8_t decodeTable[];

public:
  static void encode(char *output, const uint8_t *input, size_t length);
  static std::string encode(const uint8_t *input, size_t length);
  static void decode(uint8_t *output, const char *input, size_t length);
  static void decode(uint8_t *output, const std::string &input);

  static size_t getEncodedSize(size_t length);
  static size_t getDecodedSize(const char *input, size_t length);
  static size_t getDecodedSize(const std::string &input);
};

inline void Base64::decode(uint8_t *output, const std::string &input)
{
  decode(output, input.c_str(), input.size());
}

inline size_t Base64::getEncodedSize(size_t length)
{
  return (((length + 2) / 3) * 4);
}

inline size_t Base64::getDecodedSize(const char *input, size_t length)
{
  if(input[length - 2] == '=')
  {
    return ((length - 4) / 4) * 3 + 1;
  }
  else if(input[length - 1] == '=')
  {
    return ((length - 4) / 4) * 3 + 2;
  }
  else
  {
    return (length / 4) * 3;
  }
}

inline size_t Base64::getDecodedSize(const std::string &input)
{
  return getDecodedSize(input.c_str(), input.size());
}


#endif  // BASE64_H

