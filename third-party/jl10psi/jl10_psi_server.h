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

#ifndef JL10_SERVER_H
#define JL10_SERVER_H

#include "server.h"

class JL10_Server: public Server
{
 public:
  JL10_Server();
  ~JL10_Server();
  void LoadData(vector<string> data);
  void Setup(int nbits);
  virtual void Initialize(int nbits);
  virtual void PublishData(vector<string>& output);
  void Evaluate(mpz_t * result_l, mpz_t * result_r, mpz_t * y);
  virtual int OnRequest1(const vector<string>& input);
  void SetInputFile(string name);
  
 private:
  RandomNumber rn;
  mpz_t * m_p;
  mpz_t * m_g;
  mpz_t * m_q;
  mpz_t * m_y;
  mpz_t * m_k;
  mpz_t * m_t;
  vector<string> m_vecUi;
};

#endif
