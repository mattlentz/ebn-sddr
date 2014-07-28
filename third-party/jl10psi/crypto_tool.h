#ifndef CRYPTO_TOOL_H
#define CRYPTO_TOOL_H

#include "base.h"
#include <openssl/sha.h>
#include <openssl/rc4.h>
#include "randomnumber.h"


class CryptoTool
{
 public:
  static string Hash(const unsigned char * input, size_t len);
  static string Hash(const string& input);
  static string Hash(const mpz_t * input);
  static string FDH(const string& input, mpz_t * modulus);
  static string FDH(const mpz_t * input, mpz_t * modulus);
  static void FDH(mpz_t * output, const string& input, mpz_t * modulus);
  static void RC4(char * output, const char* input, const size_t datalen, const string& key);
  static string RC4(const string& input, const string& key);
  static string RC4(const string& input, const mpz_t * key);
  static void GeneratePrime(mpz_t * prime, int nbits);
  static void GenerateStrongPrime(mpz_t * prime, int nbits);
  static void GenerateCyclicGroup(mpz_t * g, mpz_t * p, mpz_t * q, mpz_t * x, mpz_t * h, int nbits);
  static void ElGamalEnc(mpz_t * elgamal_l, mpz_t * elgamal_r, const mpz_t * m, mpz_t * g, mpz_t * p, mpz_t * q, mpz_t * h);
  static void ElGamalDec(mpz_t * m, mpz_t * elgamal_l, mpz_t * elgamal_r, mpz_t * g, mpz_t * p, mpz_t * q, mpz_t * x);
  static void CSEnc(mpz_t * u, mpz_t * e, mpz_t * m, mpz_t * g, mpz_t * h, mpz_t * y, mpz_t * q, mpz_t * n, mpz_t * n2);
  static void CSDec(mpz_t * m, mpz_t * u, mpz_t * e, mpz_t * x, mpz_t * n, mpz_t *n2);
  static void InitRSA(struct RSA_struct * rsa);
  static void RsaSig(mpz_t * sig, mpz_t * m, RSA_struct * rsa);
  static void RsaSig(mpz_t * sig, mpz_t * m, mpz_t * d, mpz_t * n);
  static void SchnorrSig(mpz_t * X, mpz_t * e, mpz_t * s, mpz_t * hm, mpz_t * x, mpz_t *g, mpz_t * q, mpz_t * p);
  static RandomNumber rn;
};

#endif
