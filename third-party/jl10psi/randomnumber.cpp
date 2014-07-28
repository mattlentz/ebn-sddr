#include "randomnumber.h"
#include "memreuse.h"

RandomNumber::RandomNumber()
{
  gmp_randinit_default(m_rand);
}

void RandomNumber::Initialize(unsigned long nbits)
{
  char* buf;
  unsigned long bytes = (nbits + 7)/8;
  mpz_t * seed = Memreuse::New();
  buf = new char[bytes];
  get_rand_devrandom(buf, bytes);
  mpz_import(*seed, bytes, 1, 1, 0, 0, buf);
  gmp_randseed(m_rand, *seed);
  delete buf;
  Memreuse::Delete(seed);
}

void RandomNumber::get_rand_file( char* buf, int len, const char* file )
{
	FILE* fp;
	char* p;

	fp = fopen(file, "r");

	p = buf;
	while( len )
	{
		size_t s;
		s = fread(p, 1, len, fp);
		p += s;
		len -= s;
	}

	fclose(fp);
}

void RandomNumber::get_rand_devrandom( char* buf, int len )
{
	get_rand_file(buf, len, "/dev/urandom");
}

void RandomNumber::Get_Z_Range(mpz_t* ret, mpz_t * n)
{
  mpz_urandomm(*ret, m_rand, *n);
}

void RandomNumber::Get_Z_Bits(mpz_t* ret, unsigned long nbits)
{
  do
	{
	  mpz_urandomb(*ret, m_rand, nbits);
	}
  while ( !mpz_tstbit(*ret, nbits - 1) );
}
