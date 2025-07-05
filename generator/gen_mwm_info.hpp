#pragma once

#include "generator/composite_id.hpp"
#include "generator/feature_builder.hpp"

#include "coding/file_reader.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace generator
{
CompositeId MakeCompositeId(feature::FeatureBuilder const & fb);

class OsmID2FeatureID
{
public:
  enum class Version : uint8_t
  {
    V0 = 0,
    V1
  };

  OsmID2FeatureID();

  bool ReadFromFile(std::string const & filename);

  void AddIds(CompositeId const & osmId, uint32_t featureId);
  std::vector<uint32_t> GetFeatureIds(CompositeId const & id) const;
  std::vector<uint32_t> GetFeatureIds(base::GeoObjectId mainId) const;

  Version GetVersion() const;

  template <class Fn>
  void ForEach(Fn && fn) const
  {
    for (auto const & v : m_data)
      fn(v);
  }

  template <class TSink>
  void Write(TSink & sink)
  {
    std::sort(std::begin(m_data), std::end(m_data));
    WriteToSink(sink, kHeaderMagic);
    WriteToSink(sink, base::Underlying(m_version));
    rw::WriteVectorOfPOD(sink, m_data);
  }

  template <class Source>
  void ReadAndCheckHeader(Source & src)
  {
    // The first element of the old format takes more than kHeaderMagic.
    // Probably we are dealing with a empty vector of the old format.
    if (src.Size() < sizeof(kHeaderMagic))
    {
      LOG(LWARNING, ("There is insufficient file size."));
      return;
    }
    auto const headerMagic = ReadPrimitiveFromSource<uint32_t>(src);
    if (headerMagic == kHeaderMagic)
    {
      using VersionType = std::underlying_type_t<Version>;
      auto const version = static_cast<Version>(ReadPrimitiveFromSource<VersionType>(src));
      switch (version)
      {
      case Version::V1: rw::ReadVectorOfPOD(src, m_data); break;
      default: UNREACHABLE();
      }
    }
    else
    {
      // Code for supporting old format.
      src.SetPosition(0 /* position */);
      std::vector<std::pair<base::GeoObjectId, uint32_t>> data;
      rw::ReadVectorOfPOD(src, data);
      m_data.reserve(data.size());
      for (auto const & pair : data)
        m_data.emplace_back(CompositeId(pair.first), pair.second);

      m_version = Version::V0;
    }
    CHECK(base::IsSortedAndUnique(m_data), ());
  }

private:
  static uint32_t const kHeaderMagic = 0xFFFFFFFF;
  Version m_version;
  std::vector<std::pair<CompositeId, uint32_t>> m_data;
};
}  // namespace generator
