#pragma once
#include "cell_id.hpp"
#include "data_factory.hpp"
#include "feature_covering.hpp"
#include "features_vector.hpp"
#include "scale_index.hpp"
#include "mwm_set.hpp"

#include "../coding/file_container.hpp"

#include "../../defines.hpp"

#include "../std/unordered_set.hpp"
#include "../std/vector.hpp"


class MwmValue : public MwmSet::MwmValueBase
{
public:
  FilesContainerR m_cont;
  IndexFactory m_factory;

  MwmValue(string const & name);
  feature::DataHeader const & GetHeader() const { return m_factory.GetHeader(); }
};

class Index : public MwmSet
{
protected:
  virtual void GetInfo(string const & name, MwmInfo & info) const;
  virtual MwmValue * CreateValue(string const & name) const;

public:
  Index();
  ~Index();

  class MwmLock : public MwmSet::MwmLock
  {
  public:
    MwmLock(Index const & index, MwmId mwmId) : MwmSet::MwmLock(index, mwmId) {}

    inline MwmValue * GetValue() const
    {
      return static_cast<MwmValue *>(MwmSet::MwmLock::GetValue());
    }
  };

  template <typename F>
  void ForEachInRect(F & f, m2::RectD const & rect, uint32_t scale) const
  {
    ForEachInIntervals(f, covering::CoverViewportAndAppendLowerLevels(rect, RectId::DEPTH_LEVELS),
                       rect, scale);
  }

  template <typename F>
  void ForEachInRect_TileDrawing(F & f, m2::RectD const & rect, uint32_t scale) const
  {
    covering::IntervalsT intervals;
    covering::AppendLowerLevels(covering::GetRectIdAsIs(rect), RectId::DEPTH_LEVELS, intervals);
    ForEachInIntervals(f, intervals, rect, scale);
  }

  template <typename F>
  void ForEachInScale(F & f, uint32_t scale) const
  {
    covering::IntervalsT intervals;
    intervals.push_back(covering::IntervalsT::value_type(
                          0, static_cast<int64_t>((1ULL << 63) - 1)));
    ForEachInIntervals(f, intervals, m2::RectD::GetInfiniteRect(), scale);
  }

private:

  template <typename F>
  class ReadFeatureFunctor
  {
    FeaturesVector const & m_V;
    F & m_F;
    unordered_set<uint32_t> & m_offsets;

  public:
    ReadFeatureFunctor(FeaturesVector const & v, F & f, unordered_set<uint32_t> & offsets)
      : m_V(v), m_F(f), m_offsets(offsets) {}
    void operator() (uint32_t offset) const
    {
      if (m_offsets.insert(offset).second)
      {
        FeatureType feature;
        m_V.Get(offset, feature);
        m_F(feature);
      }
    }
  };


  template <typename F>
  void ForEachInIntervals(F & f, covering::IntervalsT const & intervals,
                          m2::RectD const & rect, uint32_t scale) const
  {
    vector<MwmInfo> mwm;
    GetMwmInfo(mwm);
    for (MwmId id = 0; id < mwm.size(); ++id)
    {
      if ((mwm[id].m_minScale <= scale && scale <= mwm[id].m_maxScale) &&
          rect.IsIntersect(mwm[id].m_limitRect))
      {
        MwmLock lock(*this, id);
        MwmValue * pValue = lock.GetValue();
        if (pValue)
        {
          FeaturesVector fv(pValue->m_cont, pValue->GetHeader());
          ScaleIndex<ModelReaderPtr> index(pValue->m_cont.GetReader(INDEX_FILE_TAG),
                                           pValue->m_factory);

          unordered_set<uint32_t> offsets;
          ReadFeatureFunctor<F> f1(fv, f, offsets);
          for (size_t i = 0; i < intervals.size(); ++i)
          {
            index.ForEachInIntervalAndScale(f1, intervals[i].first, intervals[i].second, scale);
          }
        }
      }
    }
  }
};
