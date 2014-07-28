#ifndef MEMREUSE_H
#define MEMREUSE_H

#include <gmp/gmp.h>
#include "base.h"

class Memreuse
{
 public:
  static void Initialize(unsigned long size, unsigned long nbits);
  static mpz_t * New();
  static void Delete(mpz_t*& mpz);
 protected:
  static queue<mpz_t* > m_qmpz;
  static unsigned long m_nbits;
};

#endif
