#include "crypto_tool.h"
#include "randomnumber.h"
#include "mpz_tool.h"

#define HASH_OUTPUT 160			// SHA1

RandomNumber CryptoTool::rn;
string CryptoTool::Hash(const unsigned char * input, size_t len)
{
  unsigned char * pch =  SHA1(input, len, NULL);
  string output((char *)pch, (size_t)160/8);
  return output;
}

string CryptoTool::Hash(const string& input)
{
  unsigned char * pch =  SHA1((const unsigned char*)input.c_str(), input.size(), NULL);
  string output((char *)pch, (size_t)160/8);
  return output;
}

string CryptoTool::Hash(const mpz_t * input)
{
  size_t bits = mpz_sizeinbase (*input, 2);
  size_t len = (bits+7)/8;
  char * input_buf = new char[len];
  mpz_export((void *)input_buf, NULL, 1, 1, 0, 0, *input);
  unsigned char * pch =  SHA1((const unsigned char*)input_buf, len, NULL);
  delete input_buf;
  string output((char *)pch, 160/8);
  return output;
}

string CryptoTool::FDH(const string& input, mpz_t * modulus)
{
  int nbits = mpz_sizeinbase (*modulus, 2);
  int repetition = (nbits + HASH_BITS_LEN - 1) / HASH_BITS_LEN;
  stringstream ss;
  stringstream result;
  for (int i = 0; i < repetition; i++)
    {
      ss.str("");
      ss<<i<<input;
      result<<Hash(ss.str());
    }
  mpz_t * tmp = Memreuse::New();
  MpzTool::Mpz_from_string(tmp, result.str());
  mpz_mod(*tmp, *tmp, *modulus);
  string rt = MpzTool::Mpz_to_string(tmp);
  Memreuse::Delete(tmp);
  return rt;
}

void CryptoTool::FDH(mpz_t * output, const string& input, mpz_t * modulus)
{
  int nbits = mpz_sizeinbase (*modulus, 2);
  int repetition = (nbits + HASH_BITS_LEN - 1) / HASH_BITS_LEN;
  stringstream ss;
  stringstream result;
  for (int i = 0; i < repetition; i++)
    {
      ss.str("");
      ss<<i<<input;
      result<<Hash(ss.str());
    }

  MpzTool::Mpz_from_string(output, result.str());
  mpz_mod(*output, *output, *modulus);
}

string CryptoTool::FDH(const mpz_t * input, mpz_t * modulus)
{
  int nbits = mpz_sizeinbase (*modulus, 2);
  int repetition = (nbits + HASH_BITS_LEN - 1) / HASH_BITS_LEN;
  stringstream ss;
  stringstream result;
  string sinput = MpzTool::Mpz_to_string(input);
  for (int i = 0; i < repetition; i++)
    {
      ss.str("");
      ss<<i<<input;
      result<<Hash(ss.str());
    }
  mpz_t * tmp = Memreuse::New();
  MpzTool::Mpz_from_string(tmp, result.str());
  mpz_mod(*tmp, *tmp, *modulus);
  string rt = MpzTool::Mpz_to_string(tmp);
  Memreuse::Delete(tmp);
  return rt;
}


string CryptoTool::RC4(const string& input, const string& key)
{
  unsigned long datalen = input.length();
  char * output = new char[datalen];	// get the same length as input
  RC4_KEY rc_key;
  RC4_set_key(&rc_key, key.length(), (const unsigned char *)key.c_str());
  ::RC4(&rc_key, datalen, (const unsigned char *)input.c_str(), (unsigned char *)output);
  string str(output, datalen);
  delete output;
  return str;
}

void CryptoTool::RC4(char * output, const char* input, const size_t datalen, const string& key)
{
  RC4_KEY rc_key;
  RC4_set_key(&rc_key, key.length(), (const unsigned char *)key.c_str());
  ::RC4(&rc_key, datalen, (const unsigned char *)input, (unsigned char *)output);
}


string CryptoTool::RC4(const string& input, const mpz_t * key)
{
  size_t bits = mpz_sizeinbase (*key, 2);
  size_t len = (bits+7)/8;
  char * buf = new char[len];
  size_t reallen = 0;
  mpz_export((void *)buf, &reallen, 1, 1, 0, 0, *key);
  assert(len == reallen);

  unsigned long datalen = input.length();
  char * output = new char[datalen];	// get the same length as input
  RC4_KEY rc_key;
  RC4_set_key(&rc_key, len, (const unsigned char *)buf);
  ::RC4(&rc_key, datalen, (const unsigned char *)input.c_str(), (unsigned char *)output);
  string str(output, datalen);
  delete output;
  delete buf;
  return str;
}

void CryptoTool::GeneratePrime(mpz_t * prime, int nbits)
{
  RandomNumber rn;
  rn.Initialize(nbits);
  rn.Get_Z_Bits(prime, nbits);

  // generate nbits long random number
  if (!mpz_tstbit(*prime, nbits-1))
	{
	  mpz_t * one = Memreuse::New();
	  mpz_set_ui(*one, 1);
	  mpz_mul_2exp(*one, *one, nbits-1);
	  mpz_ior(*prime, *prime, *one);
	  Memreuse::Delete(one);
	}
  mpz_nextprime(*prime, *prime);
}

void CryptoTool::GenerateStrongPrime(mpz_t * prime, int nbits)
{
  mpz_t * one = Memreuse::New();
  GeneratePrime(prime, nbits);
  do
	{
	  mpz_set(*one, *prime);
	  mpz_nextprime(*prime, *one);
	  mpz_tdiv_q_ui(*one, *prime, 2);
	  //gmp_printf("%Zx\n", *prime);
	}
  while(!mpz_probab_prime_p(*one, 10));
  Memreuse::Delete(one);
}

//p is modulus, q is order, g is generator, x is private key, h is public key
void CryptoTool::GenerateCyclicGroup(mpz_t * g, mpz_t * p, mpz_t * q, mpz_t * x, mpz_t * h, int nbits)
{
  RandomNumber rn;
  rn.Initialize(nbits);

  GenerateStrongPrime(p, nbits);
  mpz_tdiv_q_ui(*q, *p, 2);
  rn.Get_Z_Range(g, p);
  mpz_powm_ui(*g, *g, 2, *p);

  rn.Get_Z_Range(x, q);
  mpz_powm(*h, *g, *x, *p);
}

void CryptoTool::ElGamalEnc(mpz_t * elgamal_l, mpz_t * elgamal_r, const mpz_t * m, mpz_t * g, mpz_t * p, mpz_t * q, mpz_t * h)
{
  mpz_t * y = Memreuse::New();  
  rn.Get_Z_Range(y, q);
  mpz_powm(*elgamal_l, *g, *y, *p); // g^y
  mpz_powm(*elgamal_r, *h, *y, *p); // h^y
  mpz_mul(*elgamal_r, *elgamal_r, *m); // h^y*m
  mpz_mod(*elgamal_r, *elgamal_r, *p);
  Memreuse::Delete(y);
}

void CryptoTool::ElGamalDec(mpz_t * m, mpz_t * elgamal_l, mpz_t * elgamal_r, mpz_t * g, mpz_t * p, mpz_t * q, mpz_t * x)
{
  mpz_t * tmp = Memreuse::New();
  mpz_powm(*tmp, *elgamal_l, *x, *p);
  mpz_invert(*tmp, *tmp, *p);
  mpz_mul(*m, *tmp, *elgamal_r);
  mpz_mod(*m, *m, *p);
  Memreuse::Delete(tmp);
}

void CryptoTool::CSEnc(mpz_t * u, mpz_t * e, mpz_t * m, mpz_t * g, mpz_t * h, mpz_t * y, mpz_t * q, mpz_t * n, mpz_t * n2)
{
  mpz_t * r = Memreuse::New();
  mpz_t * hm = Memreuse::New();
  rn.Get_Z_Range(r, q);         // or mod n?
  mpz_powm(*u, *g, *r, *n2);    // u=g^r
  mpz_powm(*hm, *h, *m, *n2);   // hm=h^m
  //gmp_printf("hm=%Zx\n", *hm);
  mpz_powm(*e, *y, *r, *n2);    // e=y^r
  mpz_mul(*e, *e, *hm);         // e=y^r*h^m
  mpz_mod(*e, *e, *n2);
  Memreuse::Delete(r);
  Memreuse::Delete(hm);
}

void CryptoTool::CSDec(mpz_t * m, mpz_t * u, mpz_t * e, mpz_t * x, mpz_t * n, mpz_t *n2)
{
  mpz_t * uinv = Memreuse::New();
  mpz_t * hm = Memreuse::New();
  mpz_powm(*uinv, *u, *x, *n2); // u^x
  mpz_invert(*uinv, *uinv, *n2);	// u^-x
  mpz_mul(*hm, *e, *uinv);		   // e/u^x
  mpz_mod(*hm, *hm, *n2);
  //gmp_printf("hm=%Zx\n", *hm);
  mpz_powm_ui(*hm, *hm, 2, *n2); // (e/u^x)^2
  mpz_sub_ui(*m, *hm, 1);
  mpz_divexact(*m, *m, *n);
  mpz_set_ui(*uinv, 2);
  mpz_invert(*uinv, *uinv, *n);
  mpz_mul(*m, *m, *uinv);		// m/2
  mpz_mod(*m, *m, *n);
}

void CryptoTool::InitRSA(struct RSA_struct * rsa)
{
  mpz_t * p1 = Memreuse::New();
  mpz_t * q1 = Memreuse::New();
  if (rsa == NULL)
	{
	  cout<<"rsa empty"<<endl;
	  return;
	}
  mpz_sub_ui(*p1, *(rsa->p), 1);
  mpz_sub_ui(*q1, *(rsa->q), 1);
  mpz_mod(*(rsa->dmp1), *(rsa->d), *p1);
  mpz_mod(*(rsa->dmq1), *(rsa->d), *q1);
  mpz_invert(*(rsa->qinv), *(rsa->q), *(rsa->p));
  mpz_mul(*(rsa->order), *(p1), *(q1));
  mpz_divexact_ui(*(rsa->order), *(rsa->order), 4);
  Memreuse::Delete(p1);
  Memreuse::Delete(q1);
}

void CryptoTool::RsaSig(mpz_t * sig, mpz_t * m, RSA_struct * rsa)
{
  mpz_t * m1 = Memreuse::New();
  mpz_t * m2 = Memreuse::New();
  mpz_t * r = Memreuse::New();

  mpz_powm(*m1, *m, *(rsa->dmp1), *(rsa->p));
  mpz_powm(*m2, *m, *(rsa->dmq1), *(rsa->q));
  mpz_sub(*r, *m1, *m2);
  mpz_mod(*r, *r, *(rsa->p));
  mpz_mul(*m1, *(rsa->qinv), *r);
  mpz_mod(*m1, *m1, *(rsa->p));
  mpz_mul(*m1, *m1, *(rsa->q));
  mpz_add(*sig, *m2, *m1);

  Memreuse::Delete(m1);
  Memreuse::Delete(m2);  
  Memreuse::Delete(r);
}

void CryptoTool::RsaSig(mpz_t * sig, mpz_t * hm, mpz_t * d, mpz_t *n)
{
  mpz_powm(*sig, *hm, *d, *n);
}

void CryptoTool::SchnorrSig(mpz_t * X, mpz_t * e, mpz_t * s, mpz_t * hm, mpz_t * x, mpz_t *g, mpz_t * q, mpz_t * p)
{
  mpz_t * k = Memreuse::New();
  RandomNumber rn;
  rn.Initialize(mpz_sizeinbase(*q, 2));
  rn.Get_Z_Range(k, q);
  mpz_powm(*X, *g, *k, *p);
  
  stringstream ss;
  ss<<MpzTool::Mpz_to_string(hm)<<MpzTool::Mpz_to_string(X);
  string c = Hash(ss.str());	// 20bytes
  MpzTool::Mpz_from_string(e, c);
  mpz_mod(*e, *e, *q);
  mpz_mul(*s, *e, *x);
  mpz_mod(*s, *s, *q);
  mpz_add(*s, *k, *s);
  mpz_mod(*s, *s, *q);
  Memreuse::Delete(k);
}
