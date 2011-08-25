#include "search_index_builder.hpp"
#include "features_vector.hpp"

#include "../defines.hpp"

#include "../search/search_trie.hpp"
#include "../search/string_search_utils.hpp"

#include "../coding/trie_builder.hpp"
#include "../coding/writer.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"


namespace
{

struct FeatureName
{
  strings::UniString m_name;
  char m_Value[5];

  FeatureName(strings::UniString const & name, uint32_t id, uint8_t rank) : m_name(name)
  {
    m_Value[0] = rank;
    uint32_t const idToWrite = SwapIfBigEndian(id);
    memcpy(&m_Value[1], &idToWrite, 4);
  }

  uint32_t GetKeySize() const { return m_name.size(); }
  uint32_t const * GetKeyData() const { return m_name.data(); }
  uint32_t GetValueSize() const { return 5; }
  void const * GetValueData() const { return &m_Value; }

  inline bool operator < (FeatureName const & name) const
  {
    if (m_name != name.m_name)
      return m_name < name.m_name;
    return lexicographical_compare(&name.m_Value[0], &name.m_Value[0] + sizeof(name.m_Value),
                                   &m_Value[0], &m_Value[0] + sizeof(m_Value));
  }
};

struct FeatureNameInserter
{
  vector<FeatureName> & m_names;
  uint32_t m_pos;
  uint8_t m_rank;
  FeatureNameInserter(vector<FeatureName> & names, uint32_t pos, uint8_t rank)
    : m_names(names), m_pos(pos), m_rank(rank) {}
  bool operator()(signed char, string const & name) const
  {
    m_names.push_back(FeatureName(search::NormalizeAndSimplifyString(name), m_pos, m_rank));
    return true;
  }
};

struct FeatureInserter
{
  vector<FeatureName> & m_names;
  explicit FeatureInserter(vector<FeatureName> & names) : m_names(names) {}
  void operator() (FeatureType const & feature, uint64_t pos) const
  {
    FeatureNameInserter f(m_names, static_cast<uint32_t>(pos), feature.GetSearchRank());
    feature.ForEachNameRef(f);
  }
};

struct MaxValueCalc
{
  typedef uint8_t ValueType;

  ValueType operator() (void const * p, uint32_t size) const
  {
    ASSERT_EQUAL(size, 5, ());
    return *static_cast<uint8_t const *>(p);
  }
};

}  // unnamed namespace

void indexer::BuildSearchIndex(FeaturesVector const & featuresVector, Writer & writer)
{
  vector<FeatureName> names;
  featuresVector.ForEachOffset(FeatureInserter(names));
  sort(names.begin(), names.end());
  trie::Build(writer, names.begin(), names.end(),
              trie::builder::MaxValueEdgeBuilder<MaxValueCalc>());
}

bool indexer::BuildSearchIndexFromDatFile(string const & datFile)
{
  try
  {
    FilesContainerR readCont(datFile);

    feature::DataHeader header;
    header.Load(readCont.GetReader(HEADER_FILE_TAG));

    FeaturesVector featuresVector(readCont, header);

    FilesContainerW writeCont(datFile, FileWriter::OP_APPEND);

    vector<uint8_t> serialTrie;
    {
      MemWriter<vector<uint8_t> > writer(serialTrie);
      BuildSearchIndex(featuresVector, writer);
      reverse(serialTrie.begin(), serialTrie.end());
    }

    writeCont.GetWriter(SEARCH_INDEX_FILE_TAG).Write(&serialTrie[0], serialTrie.size());
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file: ", e.what()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error writing index file: ", e.what()));
  }

  return true;
}
