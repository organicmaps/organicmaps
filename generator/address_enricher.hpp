#pragma once
#include "generator/feature_builder.hpp"

#include "indexer/feature_utils.hpp"

#include "geometry/tree4d.hpp"

#include "coding/read_write_utils.hpp"

#include <map>

namespace generator
{

class AddressEnricher
{
  // Looks too big here? Avoiding duplicates is a thing, because we still check street name first.
  // Was 20, see Generator_Filter_NY test.
  /// @see https://github.com/organicmaps/organicmaps/pull/8502 for more threshold metrics.
  static double constexpr kDistanceThresholdM = 50.0;

public:
  struct RawEntryBase
  {
    std::string m_from, m_to, m_street, m_postcode;
    feature::InterpolType m_interpol = feature::InterpolType::None;

    /// @name Used to compare house numbers by its integer value.
    /// @{
    // 0 is a possible HN value.
    static std::pair<uint64_t, uint64_t> constexpr kInvalidRange{-1, -1};
    std::pair<uint64_t, uint64_t> GetHNRange() const;
    /// @}

    template <class TSink>
    void Save(TSink & sink) const
    {
      rw::Write(sink, m_from);
      rw::Write(sink, m_to);
      rw::Write(sink, m_street);
      rw::Write(sink, m_postcode);

      WriteToSink(sink, static_cast<uint8_t>(m_interpol));
    }

    template <class TSource>
    void Load(TSource & src)
    {
      rw::Read(src, m_from);
      rw::Read(src, m_to);
      rw::Read(src, m_street);
      rw::Read(src, m_postcode);

      m_interpol = static_cast<feature::InterpolType>(ReadPrimitiveFromSource<uint8_t>(src));
    }
  };

  struct Entry : RawEntryBase
  {
    std::vector<m2::PointD> m_points;
  };

  AddressEnricher();

  void AddSrc(feature::FeatureBuilder && fb);

  using TFBCollectFn = std::function<void(feature::FeatureBuilder &&)>;
  void ProcessRawEntries(std::string const & path, TFBCollectFn const & fn);

  // Public for tests.
  struct FoundT
  {
    int addrsInside = 0;
    bool from = false, to = false, street = false, interpol = false;
  };
  FoundT Match(Entry & e) const;

  struct Stats
  {
    uint32_t m_noStreet = 0, m_existInterpol = 0, m_existSingle = 0, m_enoughAddrs = 0;
    uint32_t m_addedSingle = 0, m_addedBegEnd = 0, m_addedInterpol = 0;

    void Add(Stats const & s)
    {
      m_noStreet += s.m_noStreet;
      m_existInterpol += s.m_existInterpol;
      m_existSingle += s.m_existSingle;
      m_enoughAddrs += s.m_enoughAddrs;

      m_addedSingle += s.m_addedSingle;
      m_addedBegEnd += s.m_addedBegEnd;
      m_addedInterpol += s.m_addedInterpol;
    }

    friend std::string DebugPrint(Stats const & s);
  } m_stats;

private:
  m4::Tree<feature::FeatureBuilder> m_srcTree;
  uint32_t m_addrType;
  std::map<feature::InterpolType, uint32_t> m_interpolType;
};

}  // namespace generator
