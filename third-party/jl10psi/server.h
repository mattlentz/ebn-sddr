#ifndef SERVER_H
#define SERVER_H

#include "base.h"

class Server
{
 protected:
  typedef int (Server::* DefOnRequest) (const vector<string>&);
  typedef int (Server::* DefResponse) (vector<string>&);
  int m_iNumIterations;
  string m_strLogFile;
  string m_strInputFile;
  bool m_adaptive;
  vector<mpz_t *> m_set;
  vector<string > m_rawset;
  vector<string > m_output;
 public:
  Server();
  virtual ~Server() { }
  vector <DefOnRequest > m_vecOnRequest;
  vector <DefResponse > m_vecResponse;
  int& Iterations() {return m_iNumIterations;}
  
  virtual void LoadData(vector<string> data) {};
  virtual void Setup(int nbits) {};
  virtual void Initialize(int nbits) {};
  virtual void PublishData(vector<string>& output){}
  /* the first round of request from client, input contains the request content */
  virtual int OnRequest1(const vector<string>&){return 0;}
  /* the second round of request from client, input contains the request content */
  virtual int OnRequest2(const vector<string>&){return 0;}
  /* the first round response from the server, output contains the response content */
  virtual int Response1(vector<string>& output){ output = m_output; return 0;}
  /* the second round response from the server, output contains the response content */
  virtual int Response2(vector<string>& output){ output = m_output; return 0;}
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
