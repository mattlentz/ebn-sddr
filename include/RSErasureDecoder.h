#ifndef RSERASUREDECODER_H
#define	RSERASUREDECODER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "BitMap.h"
#include "RSMatrix.h"

class RSErasureDecoder
{
private:
  RSMatrix matrix_;
  std::vector<std::vector<char> > partSymbols_;
  std::vector<std::vector<char*> > partSymbolPtrs_;
  std::vector<uint8_t> data_;
  BitMap received_;
  size_t numReceived_;
  bool isDecoded_;

public:
  RSErasureDecoder(const RSMatrix &matrix);
  RSErasureDecoder(RSErasureDecoder &&) = default;
  RSErasureDecoder(const RSErasureDecoder &other); 

  RSErasureDecoder& operator = (RSErasureDecoder &&) = default;
  RSErasureDecoder& operator = (const RSErasureDecoder &other); 

  size_t K() const;
  size_t M() const; 
  size_t W() const; 

  bool setSymbol(size_t index, const uint8_t *data);
  const uint8_t* decode();
  bool canDecode() const;
  bool isDecoded() const;

  void reset();
};

inline uint32_t RSErasureDecoder::K() const
{
  return matrix_.K();
}

inline uint32_t RSErasureDecoder::M() const
{
  return matrix_.M(); 
}

inline uint32_t RSErasureDecoder::W() const
{
  return matrix_.W();
}

inline bool RSErasureDecoder::canDecode() const
{
  return (numReceived_ >= matrix_.K()); 
}

inline bool RSErasureDecoder::isDecoded() const
{
  return isDecoded_;
}

#endif // RSERASUREDECODER_H
