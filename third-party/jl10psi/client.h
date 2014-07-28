#ifndef CLIENT_H
#define CLIENT_H

#include "base.h"

class Client
{
 public:
  typedef int (Client::* DefRequest) (vector<string>& );
  typedef int (Client::* DefOnResponse) (const vector<string>&);
  int m_iNumIterations;
  string m_strInputFile;
  bool m_adaptive;
  vector<mpz_t *> m_set;
  vector<string > m_rawset;
  vector<size_t > m_vecResult;	/* store index of m_rawset */
  vector<string > m_output;
  vector<string > m_data;		/* for adaptive only */
 public:
  Client();
  virtual ~Client() { }
  vector <DefRequest > m_vecRequest;
  vector <DefOnResponse > m_vecOnResponse;
  int& Iterations() {return m_iNumIterations;}
  
  virtual void LoadData(vector<string> data) {};
  virtual void Setup(int nbits) {};
  virtual void Initialize(int nbits) {};
  virtual void StoreData(const vector<string>& input) {}
  virtual int Request1(vector<string>& output) {output = m_output; return 0;};
  virtual int Request2(vector<string>& output) {output = m_output; return 0;};
  virtual int OnResponse1(const vector<string>& input) { return 0;};
  virtual int OnResponse2(const vector<string>& input) { return 0;};
  virtual void Verify();
  void SetInputFile(string name);
  size_t SetSize()
  {
	return m_set.size();
  }
  void SetAdaptive()
  {
	m_adaptive = true;
  }
};

#endif
