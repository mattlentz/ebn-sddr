#include "RSMatrix.h"

extern "C"
{
#include "jerasure/reed_sol.h"
}

using namespace std;

RSMatrix::RSMatrix(uint32_t K, uint32_t M, uint32_t W)
{
  K_ = K;
  M_ = M;
  W_ = W;

  // TODO: Can optimize by specifying a length longer than sizeof(long) for
  // cases where a given value of 'w' is used more than once
  for(int w = 4; w >= 1; w /= 2)
  {
    for(; W >= w; W -= w)
    {
      partW_.push_back(w);

      int *matrix = reed_sol_vandermonde_coding_matrix(K, M, 8 * w);
      partMatrices_.push_back(shared_ptr<int>(matrix, free));
    }
  }
}

