#ifndef SEGMENTEDBLOOMFILTER_H
#define SEGMENTEDBLOOMFILTER_H

#include <cstdint>
#include <set>
#include <vector>

#include "BloomFilter.h"

class SegmentedBloomFilter : public BloomFilter
{
private:
  size_t B_;
  std::vector<size_t> segmentSizes_;
  std::vector<size_t> segmentOffsets_;

  BitMap filled_;
  size_t numFilled_;

public:
  SegmentedBloomFilter();
  SegmentedBloomFilter(size_t N, size_t M, size_t K, size_t B, bool allOnes = false);
  SegmentedBloomFilter(size_t N, size_t M, size_t K, size_t B, const std::vector<size_t> &segmentSizes, bool allOnes = false);

  size_t B() const;
  float resetPFalse();

  size_t getSegmentSize(size_t segment) const;

  size_t filled() const;
  bool isFilled(size_t segment) const;

  void getSegment(size_t segment, uint8_t *dest, size_t offset = 0) const;
  float setSegment(size_t segment, const uint8_t *source, size_t offset = 0);
};

inline size_t SegmentedBloomFilter::B() const
{
  return B_;
}

inline float SegmentedBloomFilter::resetPFalse()
{
  float pFalseCopy = pFalse_;
  pFalse_ = 1;
  return pFalseCopy; 
}

inline size_t SegmentedBloomFilter::getSegmentSize(size_t segment) const
{
  return segmentSizes_[segment];
}

inline size_t SegmentedBloomFilter::filled() const
{
  return numFilled_;
}

inline bool SegmentedBloomFilter::isFilled(size_t segment) const
{
  return filled_.get(segment);
}

inline void SegmentedBloomFilter::getSegment(uint32_t segment, uint8_t *dest, uint32_t offset) const
{
  bits_.copyTo(dest, offset, segmentOffsets_[segment], segmentSizes_[segment]);
}

#endif  // SEGMENTEDBLOOMFILTER_H
