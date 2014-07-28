#include "client.h"

Client::Client()
{
  m_vecRequest.push_back(&Client::Request1);
  m_vecRequest.push_back(&Client::Request2);
  
  m_vecOnResponse.push_back(&Client::OnResponse1);
  m_vecOnResponse.push_back(&Client::OnResponse2);
  m_adaptive = false;
}

void Client::Verify()
{
  for (size_t i = 0; i < m_vecResult.size(); i++)
	{
	  cout<<m_rawset[m_vecResult[i]]<<endl;
	}
}

void Client::SetInputFile(string name)
{
  m_strInputFile = name;
}
