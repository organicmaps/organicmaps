#include "search_index_builder.hpp"

#include "feature_utils.hpp"
#include "features_vector.hpp"
#include "search_delimiters.hpp"
#include "search_trie.hpp"
#include "search_string_utils.hpp"
#include "string_file.hpp"
#include "classificator.hpp"

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

  void MakeValue(FeatureType const & f, feature::TypesHolder const & types,
                 uint64_t pos, ValueT & value) const
  {
    SaverT::ValueType v;
    v.m_featureId = static_cast<uint32_t>(pos);

    // get BEST geometry rect of feature
    m2::RectD const rect = f.GetLimitRect(-1);
    v.m_pt = rect.Center();
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
  FeatureInserter(StringsFile & names, serial::CodingParams const & cp)
    : m_names(names), m_valueSaver(cp)
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
      inserter.AddToken(search::CATEGORIES_LANG, search::FeatureTypeToString(types[i]));
  }
};

}  // unnamed namespace

void indexer::BuildSearchIndex(FeaturesVector const & featuresVector, Writer & writer,
                               string const & tmpFilePath)
{
  {
    StringsFile names(tmpFilePath);
    serial::CodingParams cp(search::GetCPForTrie(featuresVector.GetCodingParams()));

    featuresVector.ForEachOffset(FeatureInserter(names, cp));

    names.EndAdding();
    names.OpenForRead();

    trie::Build(writer, names.Begin(), names.End(), trie::builder::EmptyEdgeBuilder());

    // at this point all readers of StringsFile should be dead
  }

  FileWriter::DeleteFileX(tmpFilePath);
}

bool indexer::BuildSearchIndexFromDatFile(string const & fName)
{
  LOG(LINFO, ("Start building search index. Bits = ", search::POINT_CODING_BITS));

  try
  {
    Platform & pl = GetPlatform();
    string const datFile = pl.WritablePathForFile(fName);
    string const tmpFile = pl.WritablePathForFile(fName + ".search_index_2.tmp");

    {
      FilesContainerR readCont(datFile);

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
