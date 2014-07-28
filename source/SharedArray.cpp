#include "SharedArray.h"

// Specialization of toString for uint8_t arrays which prints out the contents
// in hex format
template<>
std::string SharedArray<uint8_t>::toString() const
{
  static const char hexTable[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

  std::string str(2 * size_, '0');
  for(int b = 0; b < size_; b++)
  {
    uint8_t value = arrayPtr_.get()[b];
    str[2 * b] = hexTable[value >> 4];
    str[(2 * b) + 1] = hexTable[value & 0xF];
  }
  return str;
}

