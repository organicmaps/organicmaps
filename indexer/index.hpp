#pragma once
#include "cell_id.hpp"
#include "data_factory.hpp"
#include "feature_covering.hpp"
#include "features_vector.hpp"
#include "scale_index.hpp"
#include "mwm_set.hpp"
#include "scales.hpp"

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

  inline feature::DataHeader const & GetHeader() const
  {
    return m_factory.GetHeader();
  }
};

class Index : public MwmSet
{
protected:
  /// @return mwm format version
  virtual int GetInfo(string const & name, MwmInfo & info) const;
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
    ForEachInIntervals(f, 0, rect, scale);
  }

  template <typename F>
  void ForEachInRect_TileDrawing(F & f, m2::RectD const & rect, uint32_t scale) const
  {
    ForEachInIntervals(f, 1, rect, scale);
  }

  template <typename F>
  void ForEachInScale(F & f, uint32_t scale) const
  {
    ForEachInIntervals(f, 2, m2::RectD::GetInfiniteRect(), scale);
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
  void ProcessMwm(F & f, MwmId id, covering::CoveringGetter & cov, uint32_t scale) const
  {
    MwmLock lock(*this, id);
    MwmValue * pValue = lock.GetValue();
    if (pValue)
    {
      feature::DataHeader const & header = pValue->GetHeader();

      // Prepare needed covering.
      int const lastScale = header.GetLastScale();

      // In case of WorldCoasts we should pass correct scale in ForEachInIntervalAndScale.
      if (scale > lastScale) scale = lastScale;

      // Use last coding scale for covering (see index_builder.cpp).
      covering::IntervalsT const & interval = cov.Get(lastScale);

      // prepare features reading
      FeaturesVector fv(pValue->m_cont, header);
      ScaleIndex<ModelReaderPtr> index(pValue->m_cont.GetReader(INDEX_FILE_TAG),
                                       pValue->m_factory);

      // iterate through intervals
      unordered_set<uint32_t> offsets;
      ReadFeatureFunctor<F> f1(fv, f, offsets);
      for (size_t i = 0; i < interval.size(); ++i)
      {
        index.ForEachInIntervalAndScale(f1, interval[i].first, interval[i].second, scale);
      }
    }
  }

  template <typename F>
  void ForEachInIntervals(F & f, int mode, m2::RectD const & rect, uint32_t scale) const
  {
    vector<MwmInfo> mwm;
    GetMwmInfo(mwm);

    covering::CoveringGetter cov(rect, mode);

    size_t const count = mwm.size();
    MwmId worldID[2] = { count, count };

    for (MwmId id = 0; id < count; ++id)
    {
      if ((mwm[id].m_minScale <= scale && scale <= mwm[id].m_maxScale) &&
          rect.IsIntersect(mwm[id].m_limitRect))
      {
        /// @todo It's better to avoid hacks with scale comparison.

        if (mwm[id].m_minScale > 0)
        {
          // process countries first
          ProcessMwm(f, id, cov, scale);
        }
        else
        {
          if (mwm[id].m_maxScale == scales::GetUpperScale())
            worldID[0] = id;  // store WorldCoasts to process
          else
            worldID[1] = id;  // store World to process
        }
      }
    }

    if (worldID[0] < count)
      ProcessMwm(f, worldID[0], cov, scale);

    if (worldID[1] < count)
      ProcessMwm(f, worldID[1], cov, scale);
  }
};
