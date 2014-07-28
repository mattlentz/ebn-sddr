/* ---------------------------------------------------------------------
 *
 * -- Privacy-preserving Sharing of Sensitive information Toolkit (PSST)
 *    (C) Copyright 2010 All Rights Reserved
 *
 * -- PSST routines -- Version 1.0.1 -- April, 2010
 * JL10 corresponds to protocol JL10 in the paper
 *
 * Author         : Yanbin Lu
 * Security and Privacy Research Outfit (SPROUT),
 * University of California, Irvine
 *
 * -- Bug report: send email to yanbinl@uci.edu
 * ---------------------------------------------------------------------
 *
 * -- Copyright notice and Licensing terms:
 *
 *  Redistribution  and  use in  source and binary forms, with or without
 *  modification, are  permitted provided  that the following  conditions
 *  are met:
 *
 * 1. Redistributions  of  source  code  must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce  the above copyright
 *    notice,  this list of conditions, and the  following disclaimer in
 *    the documentation and/or other materials provided with the distri-
 *    bution.
 * 3. The name of the University,  the SPROUT group,  or the names of its
 *    authors  may not be used to endorse or promote products deri-
 *    ved from this software without specific written permission.
 *
 * -- Disclaimer:
 *
 * THIS  SOFTWARE  IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,  INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO,  PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEO-
 * RY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  (IN-
 * CLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "jl10_psi_client.h"

#include "crypto_tool.h"
#include "basic_tool.h"
#include "mpz_tool.h"

JL10_Client::JL10_Client()
{
  m_g = Memreuse::New();
  m_p = Memreuse::New();
  m_q = Memreuse::New();
  m_x = Memreuse::New();
  m_t = Memreuse::New();		// (p-1)/q
}

JL10_Client::~JL10_Client()
{
  Memreuse::Delete(m_g);
  Memreuse::Delete(m_p);
  Memreuse::Delete(m_q);
  Memreuse::Delete(m_x);
  Memreuse::Delete(m_t);		// (p-1)/q
  
  for(vector<mpz_t *>::iterator it = m_vecR.begin(); it != m_vecR.end(); it++)
  {
	  Memreuse::Delete(*it);
  }
  
  for(vector<mpz_t *>::iterator it = m_vecRinv.begin(); it != m_vecRinv.end(); it++)
  {
	  Memreuse::Delete(*it);
  }
  
  for(vector<mpz_t *>::iterator it = m_set.begin(); it != m_set.end(); it++)
  {
	  Memreuse::Delete(*it);
  }
}

/*
void JL10_Client::LoadData()
{
  m_set.clear();
  m_rawset.clear();

  ifstream infile(m_strInputFile.c_str());
  if (!infile)
	{
	  cerr<<"open "<<m_strInputFile<<" error"<<endl;
	  return;
	}
  string element;
  while (getline(infile, element) && infile.good())
	{
	  m_rawset.push_back(element);
	  mpz_t * e = Memreuse::New();
	  CryptoTool::FDH(e, element, m_p);
	  //	  mpz_powm(*e, *e, *m_t, *m_p);
	  m_set.push_back(e);
	}
}
*/

void JL10_Client::LoadData(vector<string> data)
{
  m_set.clear();
  m_rawset.clear();

  for(vector<string>::iterator it = data.begin(); it != data.end(); it++)
	{
	  m_rawset.push_back(*it);
	  mpz_t * e = Memreuse::New();
	  CryptoTool::FDH(e, *it, m_p);
	  m_set.push_back(e);
	}
}

// for preset parameters 
void JL10_Client::Setup(int nbits)
{

  stringstream keyfile;
#ifdef ANDROID
  keyfile<<"/data/local/dsa."<<nbits<<".priv";
#else
  keyfile<<"dsa."<<nbits<<".priv";
#endif

  BasicTool::Load_DSA_Priv_Key(m_p, m_q, m_g, m_x, keyfile.str());
  mpz_sub_ui(*m_t, *m_p, 1);
  mpz_divexact(*m_t, *m_t, *m_q);
  //LoadData();
  rn.Initialize(nbits);
}

void JL10_Client::Initialize(int nbits)
{
  for(vector<mpz_t *>::iterator it = m_vecR.begin(); it != m_vecR.end(); it++)
  {
	  Memreuse::Delete(*it);
  }
  m_vecR.clear();
  
  for(vector<mpz_t *>::iterator it = m_vecRinv.begin(); it != m_vecRinv.end(); it++)
  {
	  Memreuse::Delete(*it);
  }
  m_vecRinv.clear();
  
  for (size_t i = 0; i < m_set.size(); i++)
	{
	  mpz_t * r = Memreuse::New();
	  mpz_t * rinv = Memreuse::New();
	  rn.Get_Z_Range(r, m_q);
	  mpz_invert(*rinv, *r, *m_q);
	  m_vecR.push_back(r);
	  m_vecRinv.push_back(rinv);
  }
}

void JL10_Client::StoreData(const vector<string>& input)
{
  if (m_adaptive)
	{
	  m_data = input;
	}
}

int JL10_Client::Request1(vector<string>& output)
{
  mpz_t * yi = Memreuse::New();
  for (size_t i = 0; i < m_set.size(); i++)
	{
	  mpz_powm(*m_set[i], *m_set[i], *m_t, *m_p);
	  mpz_t * r = m_vecR[i];
	  mpz_powm(*yi, *m_set[i], *r, *m_p);
	  output.push_back(MpzTool::Mpz_to_string(yi));
	  
	  mpz_t * rinv = m_vecRinv[i];
	  mpz_t * one = Memreuse::New();
	  mpz_mul(*one, *r, *rinv);
	  mpz_mod(*one, *one, *m_q);
	  mpz_powm(*yi, *yi, *rinv, *m_p);
      Memreuse::Delete(one);
    }  
  Memreuse::Delete(yi);
  return 0;
}

int JL10_Client::OnResponse1(const vector<string>& input)
{
  mpz_t * zi = Memreuse::New();
  mpz_t * wi = Memreuse::New();
  size_t i = 0;
  for (i = 0; i < m_set.size(); i++)
    {
      MpzTool::Mpz_from_string(zi, input[i]);
	  mpz_powm(*wi, *zi, *m_vecRinv[i], *m_p);
      m_mapTp[CryptoTool::Hash(wi)] = i;
    }
    
  m_vecResult.clear();
  if (!m_adaptive)
	{
	  map<string,size_t>::iterator it;
	  for (; i < input.size(); i++)
		{
		  //       if(input[i] == "PROFE")
		  //         {
		  //           i ++;
		  //           break;
		  //         }
		  if ((it = m_mapTp.find(input[i])) != m_mapTp.end())
			{
			  m_vecResult.push_back(it->second);
			}
		}
	}
  else
	{
	  map<string, size_t>::iterator it;
	  for (size_t i = 0; i < m_data.size(); i++)
		{
		  if ((it = m_mapTp.find(m_data[i])) != m_mapTp.end())
			{
			  m_vecResult.push_back(it->second);
			}
		}
	}
//   mpz_t * c = Memreuse::New();
//   mpz_t * cp = Memreuse::New();
//   mpz_t * s = Memreuse::New();
//   mpz_t * yi = Memreuse::New();
//   mpz_t * ti = Memreuse::New();
//   mpz_t * tip = Memreuse::New();
//   MpzTool::Mpz_from_string(c, input[i++]);
//   MpzTool::Mpz_from_string(s, input[i++]);
//   stringstream strstm;
//   bool bpass = true;
//   for (; i < input.size(); i = i + 3)
//     {
//       MpzTool::Mpz_from_string(yi, input[i]);
//       MpzTool::Mpz_from_string(zi, input[i+1]);
//       MpzTool::Mpz_from_string(ti, input[i+2]);
//       strstm<<CryptoTool::Hash(yi)<<CryptoTool::Hash(zi);
//       mpz_powm(*yi, *yi, *s, *m_p);
//       mpz_powm(*zi, *zi, *c, *m_p);
//       mpz_mul(*tip, *yi, *zi);
//       mpz_mod(*tip, *tip, *m_p);
//       if (mpz_cmp(*ti, *tip) != 0)
//         {
//           bpass = false;
//         }
//     }
//   MpzTool::Mpz_from_string(cp, CryptoTool::Hash(strstm.str()));
//   mpz_mod(*cp, *cp, *m_q);
//   if (mpz_cmp(*c, *cp) != 0)
//     {
//       bpass = false;
//     }
//   if (bpass)
//     cout<<"pass prove"<<endl;
//   else
//     cout<<"prove fail"<<endl;
  Memreuse::Delete(zi);
  Memreuse::Delete(wi);
//   Memreuse::Delete(c);
//   Memreuse::Delete(cp);
//   Memreuse::Delete(s);
//   Memreuse::Delete(yi);
//   Memreuse::Delete(ti);
//   Memreuse::Delete(tip);

  return 0;
}

