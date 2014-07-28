#include "basic_tool.h"
#include <sys/stat.h>
#include <time.h>
#include <sys/timeb.h>
#include "crypto_tool.h"

int BasicTool::Load_File(string& str, string filename)
{
  ifstream is;
  is.open(filename.c_str(), ios::binary);
  if (!is)
	{
	  cerr<<"open "<<filename<<" failed"<<endl;
	  return -1;
	}
  int length;
  char * buffer;

  // get length of file:
  is.seekg (0, ios::end);
  length = is.tellg();
  is.seekg (0, ios::beg);

  // allocate memory:
  buffer = new char [length];

  // read data as a block:
  is.read (buffer,length);
  is.close();
  str.assign(buffer, length);
  delete[] buffer;
  return 0;
}

int BasicTool::Dump_File(const string& str, string filename)
{
  ofstream os;
  os.open(filename.c_str(), ios::binary);
  if (!os)
	{
	  cerr<<"open "<<filename<<" failed"<<endl;
	  string foldername = filename.substr(0, filename.rfind('/'));
	  cout<<"create "<<foldername<<endl;
	  if (mkdir(foldername.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
		{
		  cerr<<"make "<<foldername<<" failed"<<endl;

		  return -1;
		}
	  cout<<"reopen "<<filename<<endl;
	  os.open(filename.c_str(), ios::binary);
	  if (!os)
		{
		  cerr<<"open "<<filename<<" failed again, give up"<<endl;
		}
	}
  os.write(str.c_str(), str.length());
  os.close();
  return 0;
}

int BasicTool::Count_Line(string filename)
{
  int lines = 0;
  string line;
  ifstream infile(filename.c_str());
  while( getline( infile, line ) )
	lines++;
  return lines;
}

string BasicTool::Strip(const string& str, char dem)
{
  string::size_type i = str.find(dem);
  assert(i != string::npos);
  string::size_type j = str.rfind(dem);
  assert(j != string::npos);
  string ret = str.substr(i+1, j-i-1);
  return ret;
}

int BasicTool::Load_RSA_Priv_Key(string filename, RSA_struct *& rsa)
{
  FILE * file;
  mpz_t * d, * e, * n, * p, * q, *g, * dmp1, * dmq1, *qinv, *order;

  d = Memreuse::New();
  e = Memreuse::New();
  n = Memreuse::New();
  p = Memreuse::New();
  q = Memreuse::New();
  g = Memreuse::New();
  dmp1 = Memreuse::New();
  dmq1 = Memreuse::New();
  qinv = Memreuse::New();
  order = Memreuse::New();

  file = fopen(filename.c_str(), "r");
  if (file == NULL)
	{
	  cerr<<"load rsa key failed from"<<filename<<endl;
	  return -1;
	}
  gmp_fscanf(file, "d=%Zx\n", *d);
  gmp_fscanf(file, "e=%Zx\n", *e);
  gmp_fscanf(file, "n=%Zx\n", *n);
  gmp_fscanf(file, "p=%Zx\n", *p);
  gmp_fscanf(file, "q=%Zx\n", *q);
  gmp_fscanf(file, "g=%Zx\n", *g);

  if (rsa == NULL)
	{
	  rsa = new RSA_struct;
	}
  rsa->n = n;
  rsa->d = d;
  rsa->e = e;
  rsa->p = p;
  rsa->q = q;
  rsa->g = g;
  rsa->dmp1 = dmp1;
  rsa->dmq1 = dmq1;
  rsa->qinv = qinv;
  rsa->order = order;

  CryptoTool::InitRSA(rsa);
  fclose(file);
  return 0;
}

int BasicTool::Load_RSA_Pub_Key(string filename, mpz_t * e, mpz_t * N, mpz_t * g, mpz_t * q)
{
  FILE * file;

  file = fopen(filename.c_str(), "r");
  if (file == NULL)
	{
	  cerr<<"load rsa key failed from"<<filename<<endl;
	  return -1;
	}
  gmp_fscanf(file, "e=%Zx\n", *e);
  gmp_fscanf(file, "n=%Zx\n", *N);
  if (g != NULL)
	gmp_fscanf(file, "g=%Zx\n", *g);
  if (q != NULL)
	gmp_fscanf(file, "order=%Zx\n", *q);

  fclose(file);
  return 0;
}

int BasicTool::Load_DSA_Priv_Key(mpz_t * p, mpz_t *q, mpz_t *g, mpz_t *x, string filename)
{
  FILE * pfile;
  pfile = fopen(filename.c_str(), "r");
  if (pfile == NULL)
	{
	  cerr<<"load dsa key failed from"<<filename<<endl;
	  return -1;
	}
  gmp_fscanf(pfile, "%Zx\n", *p);
  gmp_fscanf(pfile, "%Zx\n", *q);
  gmp_fscanf(pfile, "%Zx\n", *g);
  gmp_fscanf(pfile, "%Zx\n", *x);

  fclose(pfile);
  return 0;
}

int BasicTool::Load_DSA_Pub_Key(mpz_t * p, mpz_t *q, mpz_t *g, mpz_t *y, string filename)
{
  FILE * pfile;
  pfile = fopen(filename.c_str(), "r");
  if (pfile == NULL)
	{
	  cerr<<"load dsa key failed from"<<filename<<endl;
	  return -1;
	}
  if (p != NULL)
	gmp_fscanf(pfile, "%Zx\n", *p);
  if (q != NULL)
	gmp_fscanf(pfile, "%Zx\n", *q);
  if (g != NULL)
	gmp_fscanf(pfile, "%Zx\n", *g);
  if (y != NULL)
	gmp_fscanf(pfile, "%Zx\n", *y);
  fclose(pfile);
  return 0;
}

int BasicTool::Load_ElGamal_Priv_Key(mpz_t * g, mpz_t *p, mpz_t *q, mpz_t *x, mpz_t *h, string filename)
{
  FILE * pfile;
  pfile = fopen(filename.c_str(), "r");
  if (pfile == NULL)
	{
	  cerr<<"load dsa key failed from "<<filename<<endl;
	  return -1;
	}
  gmp_fscanf(pfile, "%Zx\n", *p);
  gmp_fscanf(pfile, "%Zx\n", *q);
  gmp_fscanf(pfile, "%Zx\n", *g);
  gmp_fscanf(pfile, "%Zx\n", *x);
  gmp_fscanf(pfile, "%Zx\n", *h);
  fclose(pfile);
  return 0;
}

int BasicTool::Load_ElGamal_Pub_Key(mpz_t * g, mpz_t *p, mpz_t *q, mpz_t *h, string filename)
{
  FILE * pfile;
  pfile = fopen(filename.c_str(), "r");
  if (pfile == NULL)
	{
	  cerr<<"load dsa key failed from "<<filename<<endl;
	  return -1;
	}
  gmp_fscanf(pfile, "%Zx\n", *p);
  gmp_fscanf(pfile, "%Zx\n", *q);
  gmp_fscanf(pfile, "%Zx\n", *g);
  gmp_fscanf(pfile, "%Zx\n", *h);
  fclose(pfile);

//   // verify
//   if (!mpz_probab_prime_p(*p, 10) || !mpz_probab_prime_p(*q, 10))
// 	{
// 	  cerr<<"p or q not prime"<<endl;
// 	}
//   mpz_t * dp = Memreuse::New();
//   mpz_mul_ui(*dp, *q, 2);
//   mpz_add_ui(*dp, *dp, 1);
//   if (mpz_cmp(*dp, *p) != 0)
// 	{
// 	  cerr<<"p != 2*q+1"<<endl;
// 	}
//   mpz_t * m = Memreuse::New();
//   mpz_t * gm = Memreuse::New();
//   mpz_t * el = Memreuse::New();
//   mpz_t * er = Memreuse::New();
//   mpz_set_ui(*m, 20);
//   mpz_powm(*gm, *g, *m, *p);
//   gmp_printf("%Zx\n", *gm);
//   CryptoTool::ElGamalEnc(el, er, m, g, p, q, h);
//   CryptoTool::ElGamalDec(m, el, er, g, p, q, x);
//   gmp_printf("%Zx\n", *m);
//   ///
  return 0;
}

double BasicTool::F_Time(int s)
{
	double measure;
	static struct timeb tstart,tend;
	long tdiff;
	
	if (s == START) 
	  {
		ftime(&tstart);
		return(0);
	  }
	else
	  {
		ftime(&tend);
		tdiff=(long)tend.millitm-(long)tstart.millitm;
		measure=((double)(tend.time-tstart.time))*1000+((double)tdiff);
		return measure;			// in miliseconds
	  }
}

size_t BasicTool::StringVectorSize(const vector<string>& sv)
{
  size_t s = 0;
  for (size_t i = 0; i < sv.size(); i++)
	s += sv[i].length();
  return s;
}

string BasicTool::ToLowercase(string& s)
{
  int (*pf)(int)=tolower; 
  transform(s.begin(), s.end(), s.begin(), pf); 
  return s;
}


void BasicTool::lTrim(string& s, string dem)
{
  string::size_type pos = s.find_first_not_of(dem);
  if(pos != 0) //if there are leading whitespaces erase them
   s.erase(0, pos);
}

void BasicTool::rTrim(string& s, string dem)
{
  string::size_type pos = s.find_last_not_of(dem);
  if(pos != string::npos)
	{
	  if (s.length()!=pos+1)//if there are trailing whitespaces erase them
		s.erase(pos+1);
	}
  else s="";
}

