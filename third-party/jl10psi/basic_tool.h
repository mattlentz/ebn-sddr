#ifndef BASIC_TOOL_H
#define BASIC_TOOL_H

#include "base.h"

class BasicTool
{
 public:
  static int Load_File(string& str, string filename);
  static int Dump_File(const string& str, string filename);
  static int Count_Line(string filename);
  static string Strip(const string& str, char dem);
  static int Load_RSA_Priv_Key(string filename, RSA_struct * &rsa);
  static int Load_RSA_Pub_Key(string filename, mpz_t * e, mpz_t * N, mpz_t * g = NULL, mpz_t * q = NULL);
  static int Load_DSA_Priv_Key(mpz_t * p, mpz_t *q, mpz_t *g, mpz_t *x, string filename);
  static int Load_DSA_Pub_Key(mpz_t * p, mpz_t *q, mpz_t *g, mpz_t *y, string filename);
  static int Load_ElGamal_Priv_Key(mpz_t * g, mpz_t *p, mpz_t *q, mpz_t *x, mpz_t *h, string filename);
  static int Load_ElGamal_Pub_Key(mpz_t * g, mpz_t *p, mpz_t *q, mpz_t *h, string filename);
  static double F_Time(int s);
  static size_t StringVectorSize(const vector<string>& sv);
  static string ToLowercase(string& str);
  static void lTrim(string& s, string dem);
  static void rTrim(string& s, string dem);
 public:
  static const int START = 0;
  static const int STOP = 1;
};

#endif
