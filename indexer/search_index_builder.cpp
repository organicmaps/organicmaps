#include "search_index_builder.hpp"

#include "feature_utils.hpp"
#include "features_vector.hpp"
#include "search_delimiters.hpp"
#include "search_trie.hpp"
#include "search_string_utils.hpp"
#include "string_file.hpp"
#include "classificator.hpp"
#include "feature_visibility.hpp"

#include "../defines.hpp"

#include "../platform/platform.hpp"

#include "../coding/trie_builder.hpp"
#include "../coding/writer.hpp"
#include "../coding/reader_writer_ops.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"


namespace
{

struct FeatureNameInserter
{
  StringsFile & m_names;
  StringsFile::ValueT m_val;

  FeatureNameInserter(StringsFile & names) : m_names(names) {}

  void AddToken(signed char lang, strings::UniString const & s) const
  {
    m_names.AddString(StringsFile::StringT(s, lang, m_val));
  }

  bool operator()(signed char lang, string const & name) const
  {
    strings::UniString const uniName = search::NormalizeAndSimplifyString(name);

    buffer_vector<strings::UniString, 32> tokens;
    SplitUniString(uniName, MakeBackInsertFunctor(tokens), search::Delimiters());

    /// @todo MAX_TOKENS = 32, in Query::Search we use 31 + prefix. Why 30 ???
    if (tokens.size() > 30)
    {
      LOG(LWARNING, ("Name has too many tokens:", name));
      tokens.resize(30);
    }

    for (size_t i = 0; i < tokens.size(); ++i)
      AddToken(lang, tokens[i]);

    return true;
  }
};

class FeatureInserter
{
  StringsFile & m_names;

  typedef StringsFile::ValueT ValueT;
  typedef search::trie::ValueReader SaverT;
  SaverT m_valueSaver;

  pair<int, int> m_scales;

  class CalcPolyCenter
  {
    typedef m2::PointD P;

    struct Value
    {
      Value(P const & p, double l) : m_p(p), m_len(l) {}

      bool operator< (Value const & r) const { return (m_len < r.m_len); }

      P m_p;
      double m_len;
    };

    vector<Value> m_poly;
    double m_length;

  public:
    CalcPolyCenter() : m_length(0.0) {}

    void operator() (CoordPointT const & pt)
    {
      m2::PointD p(pt.first, pt.second);

      m_length += (m_poly.empty() ? 0.0 : m_poly.back().m_p.Length(p));
      m_poly.push_back(Value(p, m_length));
    }

    P GetCenter() const
    {
      typedef vector<Value>::const_iterator IterT;

      double const l = m_length / 2.0;

      IterT e = lower_bound(m_poly.begin(), m_poly.end(), Value(m2::PointD(0, 0), l));
      if (e == m_poly.begin())
      {
        /// @todo It's very strange, but we have linear objects with zero length.
        LOG(LWARNING, ("Zero length linear object"));
        return e->m_p;
      }

      IterT b = e-1;

      double const f = (l - b->m_len) / (e->m_len - b->m_len);

      // For safety reasons (floating point calculations) do comparison instead of ASSERT.
      if (0.0 <= f && f <= 1.0)
        return (b->m_p * (1-f) + e->m_p * f);
      else
        return ((b->m_p + e->m_p) / 2.0);
    }
  };

  class CalcMassCenter
  {
    typedef m2::PointD P;
    P m_center;
    size_t m_count;

  public:
    CalcMassCenter() : m_center(0.0, 0.0), m_count(0) {}

    void operator() (P const & p1, P const & p2, P const & p3)
    {
      ++m_count;
      m_center += p1;
      m_center += p2;
      m_center += p3;
    }
    P GetCenter() const { return m_center / (3*m_count); }
  };

  m2::PointD GetCenter(FeatureType const & f) const
  {
    feature::EGeomType const type = f.GetFeatureType();
    switch (type)
    {
    case feature::GEOM_POINT:
      return f.GetCenter();

    case feature::GEOM_LINE:
      {
        CalcPolyCenter doCalc;
        f.ForEachPointRef(doCalc, FeatureType::BEST_GEOMETRY);
        return doCalc.GetCenter();
      }

    default:
      {
        ASSERT_EQUAL ( type, feature::GEOM_AREA, () );
        CalcMassCenter doCalc;
        f.ForEachTriangleRef(doCalc, FeatureType::BEST_GEOMETRY);
        return doCalc.GetCenter();
      }
    }
  }

  void MakeValue(FeatureType const & f, feature::TypesHolder const & types,
                 uint64_t pos, ValueT & value) const
  {
    SaverT::ValueType v;
    v.m_featureId = static_cast<uint32_t>(pos);

    // get BEST geometry rect of feature
    v.m_pt = GetCenter(f);
    v.m_rank = feature::GetSearchRank(types, v.m_pt, f.GetPopulation());

    // write to buffer
    PushBackByteSink<ValueT> sink(value);
    m_valueSaver.Save(sink, v);
  }

  class AvoidEmptyName
  {
    vector<uint32_t> m_vec;

  public:
    AvoidEmptyName()
    {
      char const * arr[][3] = {
        { "place", "city", ""},
        { "place", "city", "capital"},
        { "place", "town", ""},
        { "place", "county", ""},
        { "place", "state", ""},
        { "place", "region", ""}
      };

      Classificator const & c = classif();

      size_t const count = ARRAY_SIZE(arr);
      m_vec.reserve(count);

      for (size_t i = 0; i < count; ++i)
      {
        vector<string> v;
        v.push_back(arr[i][0]);
        v.push_back(arr[i][1]);
        if (strlen(arr[i][2]) > 0)
          v.push_back(arr[i][2]);

        m_vec.push_back(c.GetTypeByPath(v));
      }
    }

    bool IsExist(feature::TypesHolder const & types) const
    {
      for (size_t i = 0; i < m_vec.size(); ++i)
        if (types.Has(m_vec[i]))
          return true;

      return false;
    }
  };

public:
  FeatureInserter(StringsFile & names,
                  serial::CodingParams const & cp,
                  pair<int, int> const & scales)
    : m_names(names), m_valueSaver(cp), m_scales(scales)
  {
  }

  void operator() (FeatureType const & f, uint64_t pos) const
  {
    feature::TypesHolder types(f);

    // init inserter with serialized value
    FeatureNameInserter inserter(m_names);
    MakeValue(f, types, pos, inserter.m_val);

    // add names of the feature
    if (!f.ForEachNameRef(inserter))
    {
      static AvoidEmptyName check;
      if (check.IsExist(types))
      {
        // Do not add such features without names to suggestion list.
        return;
      }
    }

    // add names of categories of the feature
    for (size_t i = 0; i < types.Size(); ++i)
    {
      uint32_t type = types[i];

      // Leave only 2 level of type - for example, do not distinguish:
      // highway-primary-oneway or amenity-parking-fee.
      ftype::TruncValue(type, 2);

      // Do index only for visible types in mwm.
      pair<int, int> const r = feature::DrawableScaleRangeForType(type);
      if (my::between_s(m_scales.first, m_scales.second, r.first) ||
          my::between_s(m_scales.first, m_scales.second, r.second))
      {
        inserter.AddToken(search::CATEGORIES_LANG, search::FeatureTypeToString(type));
      }
    }
  }
};

}  // unnamed namespace

void indexer::BuildSearchIndex(FeaturesVector const & featuresV, Writer & writer,
                               string const & tmpFilePath)
{
  {
    StringsFile names(tmpFilePath);
    serial::CodingParams cp(search::GetCPForTrie(featuresV.GetCodingParams()));

    featuresV.ForEachOffset(FeatureInserter(names, cp, featuresV.GetScaleRange()));

    names.EndAdding();
    names.OpenForRead();

    trie::Build(writer, names.Begin(), names.End(), trie::builder::EmptyEdgeBuilder());

    // at this point all readers of StringsFile should be dead
  }

  FileWriter::DeleteFileX(tmpFilePath);
}

bool indexer::BuildSearchIndexFromDatFile(string const & fName, bool forceRebuild)
{
  LOG(LINFO, ("Start building search index. Bits = ", search::POINT_CODING_BITS));

  try
  {
    Platform & pl = GetPlatform();
    string const datFile = pl.WritablePathForFile(fName);
    string const tmpFile = pl.WritablePathForFile(fName + ".search_index_2.tmp");

    {
      FilesContainerR readCont(datFile);

      if (!forceRebuild && readCont.IsReaderExist(SEARCH_INDEX_FILE_TAG))
        return true;

      feature::DataHeader header;
      header.Load(readCont.GetReader(HEADER_FILE_TAG));

      FeaturesVector featuresVector(readCont, header);

      FileWriter writer(tmpFile);
      BuildSearchIndex(featuresVector, writer, pl.WritablePathForFile(fName + ".search_index_1.tmp"));

      LOG(LINFO, ("Search index size = ", writer.Size()));
    }

    {
      // Write to container in reversed order.
      FilesContainerW writeCont(datFile, FileWriter::OP_WRITE_EXISTING);
      FileWriter writer = writeCont.GetWriter(SEARCH_INDEX_FILE_TAG);
      rw_ops::Reverse(FileReader(tmpFile), writer);
    }

    FileWriter::DeleteFileX(tmpFile);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file: ", e.what()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error writing index file: ", e.what()));
    return false;
  }

  LOG(LINFO, ("End building search index."));
  return true;
}
