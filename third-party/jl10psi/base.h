#ifndef BASE_H
#define BASE_H

#include <vector>
#include <queue>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
using namespace std;
#include "string.h"
#include <gmp/gmp.h>
#include <assert.h>
#include "randomnumber.h"
#include "memreuse.h"

#define HASH_BITS_LEN 160

struct RSA_struct
{
  mpz_t * n;
  mpz_t * d;
  mpz_t * e;
  mpz_t * p;
  mpz_t * q;
  mpz_t * g;
  mpz_t * order;
  mpz_t * dmp1;
  mpz_t * dmq1;
  mpz_t * qinv;
};

#endif
