#ifndef RANDOM_H
#define RANDOM_H
#include "base.h"

class RandomNumber
{
 public:
  RandomNumber();
  void Initialize(unsigned long nbits);
  void get_rand_file( char* buf, int len, const char* file );
  void get_rand_devrandom( char* buf, int len );
  void Get_Z_Range(mpz_t * ret, mpz_t * n);
  void Get_Z_Bits(mpz_t * ret, unsigned long bits);
 private:
  gmp_randstate_t m_rand;
};

#endif
