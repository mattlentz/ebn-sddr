#include "Base64.h"

#include <assert.h>

using namespace std;

char Base64::encodeTable[] =
{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
  'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
  'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
  'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', '+', '/' };

uint8_t Base64::decodeTable[] =
{ 62,  0,  0,  0, 63, 52, 53, 54, 55, 56,
  57, 58, 59, 60, 61,  0,  0,  0,  0,  0,
   0,  0,  0,  1,  2,  3,  4,  5,  6,  7,
   8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 
  18, 19, 20, 21, 22, 23, 24, 25,  0,  0,
   0,  0,  0,  0, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
  42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

void Base64::encode(char *output, const uint8_t *input, size_t length)
{
  size_t bodyBlocks = length / 3; 
  for(int b = 0; b < bodyBlocks; b++)
  {
    uint32_t value = *(input++) << 16;
    value |= *(input++) << 8;
    value |= *(input++);

    *(output++) = encodeTable[value >> 18];
    *(output++) = encodeTable[(value >> 12) & 0x3F];
    *(output++) = encodeTable[(value >> 6) & 0x3F];
    *(output++) = encodeTable[value & 0x3F];
  }

  size_t endBytes = length % 3; 
  if(endBytes == 1) 
  {
    uint32_t value = *(input++) << 16;

    *(output++) = encodeTable[value >> 18];
    *(output++) = encodeTable[(value >> 12) & 0x3F];
    *(output++) = '=';
    *(output++) = '=';
  }
  else if(endBytes == 2)
  {
    uint32_t value = *(input++) << 16;
    value |= *(input++) << 8;

    *(output++) = encodeTable[value >> 18];
    *(output++) = encodeTable[(value >> 12) & 0x3F];
    *(output++) = encodeTable[(value >> 6) & 0x3F];
    *(output++) = '=';
  }
}

string Base64::encode(const uint8_t *input, uint32_t length)
{
  string output(getEncodedSize(length), '0'); 
  encode(&output[0], input, length);
  return output;
}

void Base64::decode(uint8_t *output, const char *input, size_t length)
{
  assert((length % 4) == 0);

  size_t bodyBlocks = length / 4; 
  size_t endBytes = 0; 
  if(input[length - 2] == '=')
  {
    endBytes = 2;
    bodyBlocks--;
  }
  else if(input[length - 1] == '=')
  {
    endBytes = 3;
    bodyBlocks--;
  }

  for(int b = 0; b < bodyBlocks; b++)
  {
    uint32_t value = decodeTable[*(input++) - 43] << 18;
    value |= decodeTable[*(input++) - 43] << 12;
    value |= decodeTable[*(input++) - 43] << 6;
    value |= decodeTable[*(input++) - 43];

    *(output++) = (uint8_t)(value >> 16);
    *(output++) = (uint8_t)(value >> 8);
    *(output++) = (uint8_t)value;
  }

  if(endBytes == 2)
  {
    uint32_t value = decodeTable[*(input++) - 43] << 18;
    value |= decodeTable[*(input++) - 43] << 12;

    *(output++) = (uint8_t)(value >> 16);
  }
  else if(endBytes == 3)
  {
    uint32_t value = decodeTable[*(input++) - 43] << 18;
    value |= decodeTable[*(input++) - 43] << 12;
    value |= decodeTable[*(input++) - 43] << 6;

    *(output++) = (uint8_t)(value >> 16);
    *(output++) = (uint8_t)(value >> 8);
  }
}

