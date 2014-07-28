#include "config.h"
#include "base.h"
#include <stdio.h>
#include <stdlib.h>

string Config::m_protocol = "";

int Config::Input(string filename)
{
  char buf[256];
  ifstream infile(filename.c_str());
  if (!infile)
	{
	  cerr<<"open "<<filename<<" failed"<<endl;
	  return 1;
	}
  char token[] = " =\t";
  while (infile.getline(buf, 256))
	{
	  if (strcmp (buf, "") == 0) // blank line
		continue;
      if (buf[0] == '#')        // comment line
        continue;
	  char * beforecomment = strtok(buf, "#");
	  char * field = strtok (beforecomment, token);
	  char * value = strtok (NULL, token);	  
	  if (strcmp (field, "protocol") == 0)
		{
		  Config::m_protocol = value;
		}
	}
  return 0;
}

