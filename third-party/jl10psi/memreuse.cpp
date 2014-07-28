#include "memreuse.h"
#include <stdio.h>
#include <stdlib.h>

queue<mpz_t* > Memreuse::m_qmpz;
unsigned long Memreuse::m_nbits = 1024;


void Memreuse::Initialize(unsigned long size, unsigned long nbits)
{
  m_nbits = nbits;
  for (unsigned long i = 0; i < size; i++)
	{
	  mpz_t * a = (mpz_t * ) malloc(sizeof(mpz_t) * 1);
	  mpz_init2( *a, nbits );
	  m_qmpz.push(a);
	}
}

mpz_t * Memreuse::New()
{
  if ( m_qmpz.size() == 0 )  
	{
	  mpz_t * a = (mpz_t * ) malloc(sizeof(mpz_t));
	  mpz_init2( *a, m_nbits );
	  m_qmpz.push(a);
	}

  mpz_t * ret = m_qmpz.front();
  m_qmpz.pop();
	
  return ret;
}

void Memreuse::Delete(mpz_t*& mpz)
{
  if (mpz != NULL)
	{
	  m_qmpz.push(mpz);
	  mpz = NULL;
	}
}
