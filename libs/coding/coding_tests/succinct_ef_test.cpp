#include "testing/testing.hpp"

#include "coding/succinct_mapper.hpp"
#include "coding/writer.hpp"

#include "3party/succinct/elias_fano_compressed_list.hpp"

#include <random>
#include <vector>

namespace succinct_ef_test
{
using namespace std;

template <class T>
vector<T> GetUniformValues(size_t count)
{
  // Use max - 1 because succinct makes val + 1 encoding internals.
  uniform_int_distribution<T> randDist(0, numeric_limits<T>::max() - 1);
  random_device randDevice;
  mt19937 randEngine(randDevice());

  vector<T> data(count);
  for (size_t i = 0; i < count; ++i)
    data[i] = randDist(randEngine);
  return data;
}

template <class T>
vector<T> GetNormalValues(size_t count, T mean)
{
  normal_distribution<> randDist(mean, 2);
  random_device randDevice;
  mt19937 randEngine(randDevice());

  vector<T> data(count);
  for (size_t i = 0; i < count; ++i)
  {
    // Use max - 1 because succinct makes val + 1 encoding internals.
    T constexpr const kMax = numeric_limits<T>::max() - 1;
    double d = round(randDist(randEngine));
    if (d < 0)
      d = 0;
    else if (d > kMax)
      d = kMax;
    data[i] = static_cast<T>(d);
  }
  return data;
}

template <class T>
double GetCompressionRatio(vector<T> const & data)
{
  succinct::elias_fano_compressed_list efList(data);

  vector<uint8_t> buffer;
  MemWriter writer(buffer);
  coding::Freeze(efList, writer, "");

  return data.size() * sizeof(T) / double(buffer.size());
}

UNIT_TEST(SuccinctEFList_Ratio)
{
  size_t constexpr kCount = 1 << 20;

  {
    // No need to use EFList for generic data.
    double const ratio2 = GetCompressionRatio(GetUniformValues<uint16_t>(kCount));
    TEST_LESS(ratio2, 1, ());
    LOG(LINFO, ("Uniform ratio 2:", ratio2));

    double const ratio4 = GetCompressionRatio(GetUniformValues<uint32_t>(kCount));
    TEST_LESS(ratio4, 1, ());
    LOG(LINFO, ("Uniform ratio 4:", ratio4));
  }

  {
    // EF is good for some kind of normal distribution of small values.
    double const ratio2 = GetCompressionRatio(GetNormalValues(kCount, uint16_t(128)));
    TEST_GREATER(ratio2, 1, ());
    LOG(LINFO, ("Normal ratio 2:", ratio2));

    double const ratio4 = GetCompressionRatio(GetNormalValues(kCount, uint32_t(1024)));
    TEST_GREATER(ratio4, 1, ());
    LOG(LINFO, ("Normal ratio 4:", ratio4));
  }
}

}  // namespace succinct_ef_test
