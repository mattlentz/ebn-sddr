#include "base.h"
#include "jl10_psi_client.h"
#include "jl10_psi_server.h"
#include "basic_tool.h"
#include "config.h"

int main(int argc, char * argv[])
{
  int nbits = 1024;
  string protocol, client_file, server_file;

  Memreuse::Initialize(4096, nbits);
  
  // Using 24 byte data strings, since link values in SDDR are 24 bytes
  vector<string> clientData;
  vector<string> serverData;
  clientData.push_back("SharedValue0000000000000");
  serverData.push_back("SharedValue0000000000000");
  
  int attempts = 50;
    
  for(int i = 1; i <= 256; i*=2)
  {
    // Pad on values as needed as the set size grows with each iteration
  	while(clientData.size() < i)
  	{
	  	char clientItem[24];
	  	sprintf(clientItem, "ClientOnlyValue%07d", clientData.size());
	  	clientData.push_back(string(clientItem));
	  	
	  	char serverItem[24];
	  	sprintf(serverItem, "ServerOnlyValue%07d", serverData.size());
	  	serverData.push_back(string(serverItem));
  	}
	
	  cout << "Set Size = " << i << endl;
	  cout << "---------------------------------------------------------------------" << endl;
    
    for(int j = 0; j < attempts; j++)
    {
	    Client *pc = new JL10_Client;
	    Server *ps = new JL10_Server;
		    
	    //adaptive
	    pc->SetAdaptive();
	    ps->SetAdaptive();
	    
	    vector<string> output;

	    size_t s_req = 0, s_resp = 0, s_publish = 0;

	    pc->Setup(nbits);
	    ps->Setup(nbits);

	    pc->LoadData(clientData);
	    ps->LoadData(serverData);
	    
	    BasicTool::F_Time(BasicTool::START);
	    pc->Initialize(nbits);
	    double timeClientInit = BasicTool::F_Time(BasicTool::STOP);

	    BasicTool::F_Time(BasicTool::START);
	    ps->Initialize(nbits);
	    double timeServerInit = BasicTool::F_Time(BasicTool::STOP);

	    BasicTool::F_Time(BasicTool::START);
	    
	    //adaptive
	    ps->PublishData(output);
	    s_publish = BasicTool::StringVectorSize(output);
	    pc->StoreData(output);
	    output.clear();
	    
	    (pc->*(pc->m_vecRequest[0]))(output);

	    s_req = BasicTool::StringVectorSize(output);

	    (ps->*(ps->m_vecOnRequest[0]))(output);

	    (ps->*(ps->m_vecResponse[0]))(output);

	    s_resp = BasicTool::StringVectorSize(output);

	    (pc->*(pc->m_vecOnResponse[0]))(output);
	    
	    double timeExec = BasicTool::F_Time(BasicTool::STOP);
	    
	    if(pc->m_vecResult.size() != 1)
	    {
	    	cout << "Error: Result vector is of size " << pc->m_vecResult.size() << endl;
	    }
	    else if(pc->m_vecResult[0] != 0)
	    {
	    	cout << "Error: Result vector contains incorrect entry " << pc->m_vecResult[0] << endl;
	    }
	    
	    cout << "Publish Size: " << s_publish << " bytes" << endl;
	    cout << "Request Size: " << s_req << " bytes" << endl;
	    cout << "Response Size: " << s_resp << " bytes" << endl;
	    cout << "InitClient: " << timeClientInit << endl;
	    cout << "InitServer: " << timeServerInit << endl;
	    cout << "Exec: " << timeExec << endl;
	    
	    delete pc;
	    delete ps;
	  }
	
	  cout << "---------------------------------------------------------------------" << endl;
	
  }
  
  return 0;
}
