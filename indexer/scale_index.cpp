#include "indexer/scale_index.hpp"


/// Using default one to one mapping.

uint32_t ScaleIndexBase::GetBucketsCount()
{
  return 18;
}

uint32_t ScaleIndexBase::BucketByScale(uint32_t scale)
{
  return scale;
}

pair<uint32_t, uint32_t> ScaleIndexBase::ScaleRangeForBucket(uint32_t bucket)
{
  return make_pair(bucket, bucket + 1);
}
