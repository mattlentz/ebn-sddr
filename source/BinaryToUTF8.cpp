#include "BinaryToUTF8.h"

#include <assert.h>

using namespace std; 

void BinaryToUTF8::encode(char *output, const uint8_t *input, size_t length)
{
  uint32_t value = 0;
  size_t numValueBits = 0;
  size_t inputPos = 0; // in bits
  size_t outputPos = 0; // in bytes

  while((inputPos < length) || (numValueBits > 0))
  {
    // Add more bytes to the value as necessary
    while((inputPos < length) && (numValueBits < 14))
    {
      size_t remaining = length - inputPos;

      // Full byte can be added
      if(remaining >= 8)
      {
        value = (value << 8) | input[inputPos >> 3];
        numValueBits += 8;
        inputPos += 8;
      }
      // Last few bits can be added
      else
      {
        value = (value << remaining) | (input[inputPos >> 3] & ((1 << remaining) - 1));
        numValueBits += remaining;
        inputPos += remaining; 
      }
    }

    uint8_t toEncode; 

    // We have 7 bits (or more) left to encode
    if(numValueBits >= 7)
    {
      numValueBits -= 7;
      toEncode = (value >> (numValueBits)) & 0x7F;
    }
    // Otherwise, we just add some padding (using 1's instead of 0's to avoid
    // using a two byte format encoding)
    else
    {
      size_t padding = 7 - numValueBits;
      toEncode = ((value << padding) | ((1 << padding) - 1)) & 0x7F; 
      numValueBits = 0;
    }

    // If all zeros, then we use the two byte format which will also hold the
    // next 7 bits
    if(toEncode == 0)
    {
      // Additional padding is needed for the end of the input
      if(numValueBits < 7)
      {
        size_t padding = 7 - numValueBits;
        toEncode = (value << padding) | ((1 << padding) - 1);
        numValueBits = 0;
      }
      // Otherwise, just take the next 7 bits
      else
      {
        numValueBits -= 7;
        toEncode = (value >> (numValueBits)) & 0x7F;
      }

      output[outputPos++] = 0xC2 | ((toEncode >> 6) & 0x01); // most significant bit
      output[outputPos++] = 0x80 | (toEncode & 0x3F); // remaining six bits

      assert(output[outputPos - 2] != 0xC0);
      assert(output[outputPos - 2] != 0xC1); 
      assert((output[outputPos - 2] & 0xC0) == 0xC0); 
      assert((output[outputPos - 1] & 0xC0) == 0x80); 
    }
    // Otherwise, then we just use the one byte format with a leading 0 bit
    else
    {
      output[outputPos++] = toEncode;

      assert((output[outputPos-1] & 0x80) == 0); 
    }
  }
}

string BinaryToUTF8::encode(const uint8_t *input, size_t length)
{
  string output(getNumEncodedBits(length)/ 8, '0');
  encode(&output[0], input, length);
  return output;
}

void BinaryToUTF8::decode(uint8_t *output, const char *input, size_t length)
{
  assert((length % 8) == 0);

  uint32_t value = 0;
  size_t numValueBits = 0;
  size_t outputPos = 0;

  for(int i = 0; i < (length / 8); i++) 
  {
    // Start of a two byte encoding
    if((input[i] & 0xC0) == 0xC0) 
    {
      value <<= 14; // 7 bits of 0's followed by...
      value |= (input[i++] & 0x01) << 6; // 1 bit from the first byte and ...
      value |= (input[i] & 0x3F); // 6 bits from the second byte
      numValueBits += 14;
    }
    // One byte encoding, just use 7 least significant bits
    else
    {
      value = (value << 7) | (input[i] & 0x7F);
      numValueBits += 7;
    }

    // Copying whole bytes to the output
    while(numValueBits >= 8)
    {
      numValueBits -= 8;
      output[outputPos++] = (value >> numValueBits) & 0xFF;
    }
  }

  // Copying any remaining bits to the output
  if(numValueBits > 0)
  {
    output[outputPos++] = value & ((1 << numValueBits) - 1);
  }
}

