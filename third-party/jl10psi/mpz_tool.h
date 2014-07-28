#ifndef MPZ_TOOL_H
#define MPZ_TOOL_H

#include "base.h"

class MpzTool
{
 public:
  static void Mpz_from_string(mpz_t * output, const string& input);
  static void Mpz_from_char(mpz_t * output, const char * input, size_t len);
  static string Mpz_to_string(const mpz_t * input);
  static string String_from_mpz(string* output, const mpz_t * input);
  static void Char_from_mpz(char* output, size_t& len, const size_t ilen, const mpz_t * input);
};

#endif
