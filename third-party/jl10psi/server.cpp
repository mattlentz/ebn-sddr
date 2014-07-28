#include "server.h"

Server::Server()
{
  m_vecOnRequest.push_back(&Server::OnRequest1);
  m_vecOnRequest.push_back(&Server::OnRequest2);
  m_vecResponse.push_back(&Server::Response1);
  m_vecResponse.push_back(&Server::Response2);
  m_adaptive = false;
}

void Server::SetInputFile(string name)
{
  m_strInputFile = name;
}
