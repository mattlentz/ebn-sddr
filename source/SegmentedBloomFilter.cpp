#include "SegmentedBloomFilter.h"

#include <assert.h>
#include <cmath>
#include <cstring>

using namespace std;

SegmentedBloomFilter::SegmentedBloomFilter()
   : BloomFilter(),
     B_(0),
     segmentSizes_(),
     segmentOffsets_(),
     filled_(),
     numFilled_(0)
{
}

SegmentedBloomFilter::SegmentedBloomFilter(size_t N, size_t M, size_t K, size_t B, bool allOnes)
   : BloomFilter(N, M, K, NULL),
     B_(0),
     segmentSizes_(B, (M / B)),
     segmentOffsets_(B, (M / B)),
     filled_(B),
     numFilled_(0)
{
  assert((M % B) == 0);

  for(size_t b = 0; b < B; b++)
  {
    segmentOffsets_[b] *= b;
  }

  if(allOnes) 
  {
    bits_.setAll(true);
    pFalse_ = 1;
  }
}

SegmentedBloomFilter::SegmentedBloomFilter(size_t N, size_t M, size_t K, size_t B, const vector<size_t>& segmentSizes, bool allOnes)
   : BloomFilter(N, M, K, NULL),
     B_(0),
     segmentSizes_(segmentSizes),
     segmentOffsets_(),
     filled_(B),
     numFilled_(0) 
{
  size_t curSegmentOffset = 0;
  for(auto segIt = segmentSizes.cbegin(); segIt != segmentSizes.cend(); segIt++)
  {
    segmentOffsets_.push_back(curSegmentOffset); 
    curSegmentOffset += *segIt; 
  }

  assert(curSegmentOffset == M); 

  if(allOnes) 
  {
    bits_.setAll(true);
    pFalse_ = 1;
  }
}

float SegmentedBloomFilter::setSegment(size_t segment, const uint8_t *source, size_t offset)
{
  bits_.copyFrom(source, offset, segmentOffsets_[segment], segmentSizes_[segment]);

  float pFalseDelta = 1;
  if(!filled_.getThenSet(segment))
  {
    pFalseDelta = pow(1 - exp(-(float)K_ * N_ / M_), ((float)segmentSizes_[segment] / M_) * K_); 
  }

  pFalse_ *= pFalseDelta;

  return pFalseDelta;
}

