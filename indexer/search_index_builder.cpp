#include "search_index_builder.hpp"

#include "feature_utils.hpp"
#include "features_vector.hpp"
#include "search_delimiters.hpp"
#include "search_trie.hpp"
#include "search_string_utils.hpp"
#include "string_file.hpp"
#include "classificator.hpp"
#include "feature_visibility.hpp"
#include "categories_holder.hpp"

#include "../search/search_common.hpp"    // for MAX_TOKENS constant

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

    int const maxTokensCount = search::MAX_TOKENS - 1;
    if (tokens.size() > maxTokensCount)
    {
      LOG(LWARNING, ("Name has too many tokens:", name));
      tokens.resize(maxTokensCount);
    }

    for (size_t i = 0; i < tokens.size(); ++i)
      AddToken(lang, tokens[i]);

    return true;
  }
};

class FeatureInserter
{
  StringsFile & m_names;

  CategoriesHolder const & m_categories;

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

  /// There are 3 different ways of search index skipping: \n
  /// - skip features in any case (m_skipFeatures) \n
  /// - skip features with empty names (m_enFeature) \n
  /// - skip specified types for features with empty names (m_enTypes) \n
  class SkipIndexing
  {
    /// Array index (0, 1) means type level for checking (1, 2).
    vector<uint32_t> m_enFeature[2];

    template <size_t count, size_t ind>
    void FillEnFeature(char const * (& arr)[count][ind])
    {
      STATIC_ASSERT ( count > 0 );
      STATIC_ASSERT ( ind > 0 );

      Classificator const & c = classif();

      m_enFeature[ind-1].reserve(count);
      for (size_t i = 0; i < count; ++i)
      {
        vector<string> v(arr[i], arr[i] + ind);
        m_enFeature[ind-1].push_back(c.GetTypeByPath(v));
      }
    }

    vector<uint32_t> m_skipFeatures, m_enTypes;

    template <size_t count, size_t ind>
    static void FillTypes(char const * (& arr)[count][ind], vector<uint32_t> & dest)
    {
      STATIC_ASSERT ( count > 0 );
      STATIC_ASSERT ( ind > 0 );

      Classificator const & c = classif();

      for (size_t i = 0; i < count; ++i)
      {
        vector<string> v(arr[i], arr[i] + ind);
        dest.push_back(c.GetTypeByPath(v));
      }
    }

  public:
    SkipIndexing()
    {
      // Get features which shoud be skipped in any case.
      char const * arrSkip2[][2] = {
        { "building", "address" }
      };

      FillTypes(arrSkip2, m_skipFeatures);

      // Get features which shoud be skipped without names.
      char const * arrEnFeature1[][1] = {
        { "highway" }, { "natural" }, { "waterway"}, { "landuse" }
      };

      char const * arrEnFeature2[][2] = {
        { "place", "city" },
        { "place", "town" },
        { "place", "county" },
        { "place", "state" },
        { "place", "region" },
        { "railway", "rail" }
      };

      FillEnFeature(arrEnFeature1);
      FillEnFeature(arrEnFeature2);

      // Get types which shoud be skipped for features without names.
      char const * arrEnTypes1[][1] = { { "building" } };

      FillTypes(arrEnTypes1, m_enTypes);
    }

    bool SkipFeature(feature::TypesHolder const & types) const
    {
      for (size_t i = 0; i < m_skipFeatures.size(); ++i)
        if (types.Has(m_skipFeatures[i]))
          return true;
      return false;
    }

    bool SkipEmptyNameFeature(feature::TypesHolder const & types) const
    {
      for (size_t i = 0; i < types.Size(); ++i)
        for (size_t j = 0; j < ARRAY_SIZE(m_enFeature); ++j)
        {
          uint32_t type = types[i];
          ftype::TruncValue(type, j+1);

          if (find(m_enFeature[j].begin(), m_enFeature[j].end(), type) != m_enFeature[j].end())
            return true;
        }

      return false;
    }

    void SkipEmptyNameTypes(feature::TypesHolder & types)
    {
      for (size_t i = 0; i < m_enTypes.size(); ++i)
        types.Remove(m_enTypes[i]);
    }
  };

public:
  FeatureInserter(StringsFile & names,
                  CategoriesHolder const & catHolder,
                  serial::CodingParams const & cp,
                  pair<int, int> const & scales)
    : m_names(names), m_categories(catHolder), m_valueSaver(cp), m_scales(scales)
  {
  }

  void operator() (FeatureType const & f, uint64_t pos) const
  {       
    feature::TypesHolder types(f);

    static SkipIndexing skipIndex;
    if (skipIndex.SkipFeature(types))
    {
      // Do not add this features in any case.
      return;
    }

    // init inserter with serialized value
    FeatureNameInserter inserter(m_names);
    MakeValue(f, types, pos, inserter.m_val);

    // add names of the feature
    if (!f.ForEachNameRef(inserter))
    {
      if (skipIndex.SkipEmptyNameFeature(types))
      {
        // Do not add features without names to index.
        return;
      }

      // Skip types for features without names.
      skipIndex.SkipEmptyNameTypes(types);
    }

    if (types.Empty()) return;

    // add names of categories of the feature
    for (size_t i = 0; i < types.Size(); ++i)
    {
      uint32_t type = types[i];

      // Leave only 2 level of type - for example, do not distinguish:
      // highway-primary-oneway or amenity-parking-fee.
      ftype::TruncValue(type, 2);

      // Push to index only categorized types.
      if (!m_categories.IsTypeExist(type))
        continue;

      // Do index only for visible types in mwm.
      pair<int, int> const r = feature::GetDrawableScaleRange(type);
      if (my::between_s(m_scales.first, m_scales.second, r.first) ||
          my::between_s(m_scales.first, m_scales.second, r.second))
      {
        inserter.AddToken(search::CATEGORIES_LANG, search::FeatureTypeToString(type));
      }
    }
  }
};

void BuildSearchIndex(FilesContainerR const & cont, CategoriesHolder const & catHolder,
                      Writer & writer, string const & tmpFilePath)
{
  {
    feature::DataHeader header;
    header.Load(cont.GetReader(HEADER_FILE_TAG));
    FeaturesVector featuresV(cont, header);

    serial::CodingParams cp(search::GetCPForTrie(header.GetDefCodingParams()));

    StringsFile names(tmpFilePath);

    featuresV.ForEachOffset(FeatureInserter(names, catHolder, cp, header.GetScaleRange()));

    names.EndAdding();
    names.OpenForRead();

    trie::Build(writer, names.Begin(), names.End(), trie::builder::EmptyEdgeBuilder());

    // at this point all readers of StringsFile should be dead
  }

  FileWriter::DeleteFileX(tmpFilePath);
}

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

      FileWriter writer(tmpFile);

      CategoriesHolder catHolder(pl.GetReader(SEARCH_CATEGORIES_FILE_NAME));

      BuildSearchIndex(readCont, catHolder, writer,
                       pl.WritablePathForFile(fName + ".search_index_1.tmp"));

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
