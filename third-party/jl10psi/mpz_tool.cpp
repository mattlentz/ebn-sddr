#include "mpz_tool.h"

void MpzTool::Mpz_from_string(mpz_t * output, const string& input)
{
  mpz_import(*output, input.length(), 1, sizeof(char), 0, 0, 
			 (const unsigned char *)input.c_str());
}

void MpzTool::Mpz_from_char(mpz_t * output, const char * input, size_t len)
{
  mpz_import(*output, len, 1, sizeof(char), 0, 0, 
			 (const unsigned char *)input);
}

string MpzTool::Mpz_to_string(const mpz_t * input)
{
  string output;
  size_t len = (mpz_sizeinbase (*input, 2) + 7)/8;
  char * buf = new char[len];
  size_t realen;
  mpz_export((void *)buf, &realen, 1, sizeof(char), 0, 0, *input);
  assert(len == realen);
  output.assign(buf, len);
  delete buf;
  return output;  
}

string MpzTool::String_from_mpz(string * poutput, const mpz_t * input)
{
  string output;
  size_t len = (mpz_sizeinbase (*input, 2) + 7)/8;
  char * buf = new char[len];
  size_t realen;  
  mpz_export((void *)buf, &realen, 1, sizeof(char), 0, 0, *input);
  assert(len == realen);
  output.assign(buf, len);
  delete buf;
  if (poutput != NULL)
	{
	  *poutput = output;
	}
  return output;
}

void MpzTool::Char_from_mpz(char* output, size_t& nlen, const size_t ilen, const mpz_t * input)
{
  size_t len = (mpz_sizeinbase (*input, 2) + 7)/8;
  if (ilen < len)
	{
	  cerr<<"Char_from_mpz output buf too small"<<endl;
	  return;
	}
  mpz_export((void *)output, &nlen, 1, sizeof(char), 0, 0, *input);
}
