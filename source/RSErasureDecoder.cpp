#include "RSErasureDecoder.h"

extern "C"
{
#include "jerasure/jerasure.h"
#include "jerasure/reed_sol.h"
}

#include "Logger.h"

using namespace std;

RSErasureDecoder::RSErasureDecoder(const RSMatrix &matrix)
   : matrix_(matrix),
     partSymbols_(matrix.getNumParts()),
     partSymbolPtrs_(matrix.getNumParts()),
     data_(matrix.K() * matrix.W(), 0),
     received_(matrix.K() + matrix.M()),
     numReceived_(0),
     isDecoded_(false)
{
  received_.setAll(false);

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

RSErasureDecoder::RSErasureDecoder(const RSErasureDecoder &other)
   : matrix_(other.matrix_),
     partSymbols_(other.partSymbols_),
     partSymbolPtrs_(matrix_.getNumParts()),
     data_(other.data_),
     received_(other.received_),
     numReceived_(other.numReceived_),
     isDecoded_(other.isDecoded_)
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

RSErasureDecoder& RSErasureDecoder::operator = (const RSErasureDecoder &other)
{
  matrix_ = other.matrix_;
  partSymbols_ = other.partSymbols_;
  partSymbolPtrs_ = vector<vector<char*> >(matrix_.getNumParts());
  data_ = other.data_;
  received_ = other.received_;
  numReceived_ = other.numReceived_;
  isDecoded_ = other.isDecoded_;

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

bool RSErasureDecoder::setSymbol(size_t index, const uint8_t *data)
{
  if(!received_.getThenSet(index))
  {
    for(size_t p = 0; p < matrix_.getNumParts(); p++)
    {
      long symbol = 0;
      for(size_t b = 0; b < matrix_.getPartW(p); b++)
      {
        symbol |= *(data++) << (8 * b);
      }
      *((long *)&partSymbols_[p][index * sizeof(long)]) = symbol;
    }

    numReceived_++;
  }

  return canDecode();
}

const uint8_t* RSErasureDecoder::decode()
{
  uint8_t *decoded = NULL;

  if(isDecoded())
  {
    decoded = data_.data();
  }
  else if(canDecode())
  {
    vector<int> erasures;
    for(size_t s = 0; s < (matrix_.K() + matrix_.M()); s++)
    {
      if(!received_.get(s))
      {
        erasures.push_back(s);
      }
    }
    erasures.push_back(-1);

    for(size_t p = 0; p < matrix_.getNumParts(); p++)
    {
      jerasure_matrix_decode(matrix_.K(), matrix_.M(), 8 * matrix_.getPartW(p), matrix_.getMatrix(p), 1, erasures.data(), partSymbolPtrs_[p].data(), &partSymbolPtrs_[p][matrix_.K()], sizeof(long));
    }

    uint8_t *pData = data_.data();
    for(size_t s = 0; s < matrix_.K(); s++)
    {
      for(size_t p = 0; p < matrix_.getNumParts(); p++)
      {
        long symbol = *((long *)&partSymbols_[p][s * sizeof(long)]);
        for(size_t b = 0; b < matrix_.getPartW(p); b++)
        {
          *(pData++) = (symbol >> (8 * b)) & 0xFF;
        }
      }
    }

    isDecoded_ = true;
    decoded = data_.data();
  }

  return decoded;
}

void RSErasureDecoder::reset()
{
  received_.setAll(false);
  numReceived_ = 0;
  isDecoded_ = false;
}

