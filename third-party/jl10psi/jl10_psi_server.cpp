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

#include "jl10_psi_server.h"
#include "crypto_tool.h"
#include "basic_tool.h"
#include "mpz_tool.h"

JL10_Server::JL10_Server()
{
  m_p = Memreuse::New();
  m_g = Memreuse::New();
  m_q = Memreuse::New();
  m_y = Memreuse::New();
  m_k = Memreuse::New();
  m_t = Memreuse::New();		// (p-1)/q
}

JL10_Server::~JL10_Server()
{
  Memreuse::Delete(m_p);
  Memreuse::Delete(m_g);
  Memreuse::Delete(m_q);
  Memreuse::Delete(m_y);
  Memreuse::Delete(m_k);
  Memreuse::Delete(m_t);		// (p-1)/q
  
  for(vector<mpz_t *>::iterator it = m_set.begin(); it != m_set.end(); it++)
  {
	  Memreuse::Delete(*it);
  }
}

/*
void JL10_Server::LoadData()
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
	  m_set.push_back(e);
	}
}
*/

void JL10_Server::LoadData(vector<string> data)
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

void JL10_Server::Setup(int nbits)
{
  stringstream keyfile;
#ifdef ANDROID
  keyfile<<"/data/local/dsa."<<nbits<<".pub";
#else
  keyfile<<"dsa."<<nbits<<".pub";
#endif

  BasicTool::Load_DSA_Pub_Key(m_p, m_q, m_g, m_y, keyfile.str());
  mpz_sub_ui(*m_t, *m_p, 1);
  mpz_divexact(*m_t, *m_t, *m_q);
  //LoadData();
  rn.Initialize(nbits);
}

void JL10_Server::Initialize(int nbits)
{
  rn.Get_Z_Range(m_k, m_q);
  mpz_t * ui = Memreuse::New();
  for (size_t i = 0; i < m_set.size(); i++)
	{
	  mpz_powm(*m_set[i], *m_set[i], *m_t, *m_p);
	  mpz_powm(*ui, *m_set[i], *m_k, *m_p);
	  m_vecUi.push_back(CryptoTool::Hash(ui));
	}
  Memreuse::Delete(ui);
}

void JL10_Server::PublishData(vector<string>& output)
{
  if (m_adaptive)
	output.insert(output.end(), m_vecUi.begin(), m_vecUi.end());

}

int JL10_Server::OnRequest1(const vector<string>& input)
{
  m_output.clear();
  mpz_t * yi = Memreuse::New();
  mpz_t * zi = Memreuse::New();
  vector<string > PoK;
  mpz_t * r = Memreuse::New();
  mpz_t * ti = Memreuse::New();
  mpz_t * c = Memreuse::New();
  mpz_t * s = Memreuse::New();
  rn.Get_Z_Range(r, m_q);
  stringstream strstm;
  for (size_t i = 0; i < input.size(); i++)
	{
	  MpzTool::Mpz_from_string(yi, input[i]);
	  mpz_powm(*zi, *yi, *m_k, *m_p);
	  m_output.push_back(MpzTool::Mpz_to_string(zi));

//       PoK.push_back(MpzTool::Mpz_to_string(yi));
//       PoK.push_back(MpzTool::Mpz_to_string(zi));
//       mpz_powm(*ti, *yi, *r, *m_p);
//       PoK.push_back(MpzTool::Mpz_to_string(ti));
//       strstm<<CryptoTool::Hash(yi)<<CryptoTool::Hash(zi);
	}
//   MpzTool::Mpz_from_string(c, CryptoTool::Hash(strstm.str()));
//   mpz_mod(*c, *c, *m_q);
//   mpz_mul(*s, *c, *m_k);
//   mpz_mod(*s, *s, *m_q);
//   mpz_sub(*s, *r, *s);
//   mpz_mod(*s, *s, *m_q);
  if (!m_adaptive)
	m_output.insert(m_output.end(), m_vecUi.begin(), m_vecUi.end());

//   m_output.push_back("PROFE");
//   m_output.push_back(MpzTool::Mpz_to_string(c));
//   m_output.push_back(MpzTool::Mpz_to_string(s));
//   m_output.insert(m_output.end(), PoK.begin(), PoK.end());
  Memreuse::Delete(yi);
  Memreuse::Delete(zi);
  Memreuse::Delete(r);
  Memreuse::Delete(ti);
  Memreuse::Delete(c);
  Memreuse::Delete(s);

 return 0;
}
