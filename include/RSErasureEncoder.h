#ifndef RSERASUREENCODER_H
#define	RSERASUREENCODER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "RSMatrix.h"

class RSErasureEncoder
{
private:
  RSMatrix matrix_;
  std::vector<std::vector<char> > partSymbols_;
  std::vector<std::vector<char*> > partSymbolPtrs_;
  std::vector<uint8_t> symbols_;

public:
  RSErasureEncoder(const RSMatrix &matrix);
  RSErasureEncoder(RSErasureEncoder &&) = default; 
  RSErasureEncoder(const RSErasureEncoder &other); 

  RSErasureEncoder& operator = (RSErasureEncoder &&) = default;
  RSErasureEncoder& operator = (const RSErasureEncoder &other); 

  size_t K() const; 
  size_t M() const; 
  size_t W() const; 

  void encode(const uint8_t *data);
  const uint8_t* getSymbol(size_t index) const;
};

inline size_t RSErasureEncoder::K() const
{
  return matrix_.K();
}

inline size_t RSErasureEncoder::M() const
{
  return matrix_.M(); 
}

inline size_t RSErasureEncoder::W() const
{
  return matrix_.W(); 
}

inline const uint8_t* RSErasureEncoder::getSymbol(size_t index) const
{
  return &symbols_[index * matrix_.W()]; 
}

#endif // RSERASUREENCODER_H
