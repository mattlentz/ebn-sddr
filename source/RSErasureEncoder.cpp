#include "RSErasureEncoder.h"

extern "C"
{
#include "jerasure/jerasure.h"
#include "jerasure/reed_sol.h"
}

using namespace std;

RSErasureEncoder::RSErasureEncoder(const RSMatrix &matrix)
   : matrix_(matrix),
     partSymbols_(matrix.getNumParts()),
     partSymbolPtrs_(matrix.getNumParts()),
     symbols_((matrix.K() + matrix.M()) * matrix.W(), 0)
{
  size_t KM = matrix.K() + matrix.M();
  for(size_t p = 0; p < matrix.getNumParts(); p++)
  {
    partSymbols_[p].resize(KM * sizeof(long));
    partSymbolPtrs_[p].resize(KM);
    for(size_t s = 0; s < KM; s++)
    {
      partSymbolPtrs_[p][s] = &partSymbols_[p][s * sizeof(long)];
    }
  }
}

RSErasureEncoder::RSErasureEncoder(const RSErasureEncoder &other)
   : matrix_(other.matrix_),
     partSymbols_(other.partSymbols_),
     partSymbolPtrs_(matrix_.getNumParts()),
     symbols_(other.symbols_)
{
  size_t KM = matrix_.K() + matrix_.M();
  for(size_t p = 0; p < matrix_.getNumParts(); p++)
  {
    partSymbolPtrs_[p].resize(KM);
    for(size_t s = 0; s < KM; s++)
    {
      partSymbolPtrs_[p][s] = &partSymbols_[p][s * sizeof(long)];
    }
  }
}

RSErasureEncoder& RSErasureEncoder::operator = (const RSErasureEncoder &other)
{
  matrix_ = other.matrix_;
  partSymbols_ = other.partSymbols_;
  partSymbolPtrs_ = vector<vector<char*> >(matrix_.getNumParts());
  symbols_ = other.symbols_;

  size_t KM = matrix_.K() + matrix_.M();
  for(size_t p = 0; p < matrix_.getNumParts(); p++)
  {
    partSymbolPtrs_[p].resize(KM);
    for(size_t s = 0; s < KM; s++)
    {
      partSymbolPtrs_[p][s] = &partSymbols_[p][s * sizeof(long)];
    }
  }

  return *this;
}

void RSErasureEncoder::encode(const uint8_t *data)
{
  for(size_t k = 0; k < matrix_.K(); k++)
  {
    for(size_t p = 0; p < matrix_.getNumParts(); p++)
    {
      long symbol = 0;
      for(size_t b = 0; b < matrix_.getPartW(p); b++)
      {
        symbol |= *(data++) << (8 * b);
      }
      *((long *)&partSymbols_[p][k * sizeof(long)]) = symbol;
    }
  }

  for(size_t p = 0; p < matrix_.getNumParts(); p++)
  {
    jerasure_matrix_encode(matrix_.K(), matrix_.M(), 8 * matrix_.getPartW(p), matrix_.getMatrix(p), partSymbolPtrs_[p].data(), &partSymbolPtrs_[p][matrix_.K()], sizeof(long));
  }

  uint8_t *pSymbols = symbols_.data();
  for(size_t s = 0; s < (matrix_.K() + matrix_.M()); s++)
  {
    for(size_t p = 0; p < matrix_.getNumParts(); p++)
    {
      long symbol = *((long *)&partSymbols_[p][s * sizeof(long)]);
      for(size_t b = 0; b < matrix_.getPartW(p); b++)
      {
        *(pSymbols++) = (symbol >> (8 * b)) & 0xFF;
      }
    }
  }
}

