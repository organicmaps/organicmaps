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
  inline feature::DataHeader const & GetHeader() const
  {
    return m_factory.GetHeader();
  }
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

  class CoveringGetter
  {
    typedef covering::IntervalsT ResT;
    ResT m_res[2];

    m2::RectD const & m_rect;
    int m_mode;

  public:
    /// @param[in] mode\n
    /// - 0 - cover viewport with low lovels;\n
    /// - 1 - cover append low levels only;\n
    /// - 2 - make full cover\n
    CoveringGetter(m2::RectD const & r, int mode) : m_rect(r), m_mode(mode) {}

    ResT const & Get(feature::DataHeader const & header);
  };

  template <typename F>
  void ForEachInIntervals(F & f, int mode, m2::RectD const & rect, uint32_t scale) const
  {
    vector<MwmInfo> mwm;
    GetMwmInfo(mwm);

    CoveringGetter cov(rect, mode);

    for (MwmId id = 0; id < mwm.size(); ++id)
    {
      if ((mwm[id].m_minScale <= scale && scale <= mwm[id].m_maxScale) &&
          rect.IsIntersect(mwm[id].m_limitRect))
      {
        MwmLock lock(*this, id);
        MwmValue * pValue = lock.GetValue();
        if (pValue)
        {
          feature::DataHeader const & header = pValue->GetHeader();

          // prepare needed covering
          covering::IntervalsT const & interval = cov.Get(header);

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
    }
  }
};
