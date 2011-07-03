#include "sloynik_index.hpp"
#include "dictionary.hpp"

#include "../platform/platform.hpp"

#include "../coding/file_writer.hpp"
#include "../coding/timsort/timsort.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"
#include "../base/cache.hpp"

#include "../std/bind.hpp"


#define FILE_SORTER_LOG_BATCH_SIZE 11

sl::SortedIndex::SortedIndex(Dictionary const & dictionary,
                             Reader const * pIndexReader,
                             StrFn const & strFn)
  : m_Dictionary(dictionary), m_StrFn(strFn),
    m_SortedVector( PolymorphReader(pIndexReader),
                    static_cast<DDVectorType::size_type>(pIndexReader->Size() / sizeof(DicId)))
{
  STATIC_ASSERT(sizeof(sl::SortedIndex::DicId) == 3);
}

sl::Dictionary const & sl::SortedIndex::GetDictionary() const
{
  return m_Dictionary;
}

sl::Dictionary::Id sl::SortedIndex::KeyIdByPos(Pos pos) const
{
  return m_SortedVector[pos].Value();
}

namespace sl
{
namespace impl
{

  #define DIC_ID_MAGIC_EMPTY_VALUE 0xFFFFFF

  // Compare 2 values of Dictionary::Id using comparator provided.
  // A special value of Dictionary::Id(DIC_ID_MAGIC_EMPTY_VALUE) is used
  // to compare with a string provided in constructor.
  template <bool bFullCompare>
  class DictCollIdLess
  {
  private:
    DictCollIdLess(DictCollIdLess<bFullCompare> const &);
    DictCollIdLess<bFullCompare> & operator = (DictCollIdLess<bFullCompare> const &);
  public:
    DictCollIdLess(sl::Dictionary const & dic, sl::StrFn const & strFn)
    : m_Dic(dic), m_StrFn(strFn), m_Special(NULL), m_Cache(FILE_SORTER_LOG_BATCH_SIZE + 1)
    {
    }

    DictCollIdLess(sl::Dictionary const & dic, sl::StrFn const & strFn, string const & special)
    : m_Dic(dic), m_StrFn(strFn), m_Special(strFn.Create(&special[0], special.size())), m_Cache(3)
    {
    }

    ~DictCollIdLess()
    {
      if (m_Special)
        m_StrFn.Destroy(m_Special);
      m_Cache.ForEachValue(m_StrFn.Destroy);
    }

    int Compare(uint32_t const id1, uint32_t const id2)
    {
      if (id1 == id2)
        return 0;
      sl::StrFn::Str const * key1, * key2;
      bool bDeleteKey1 = false;
      KeysById(id1, id2, key1, key2, bDeleteKey1);
      int const primaryRes = m_StrFn.PrimaryCompare(m_StrFn.m_pData, key1, key2);
      if (bFullCompare && primaryRes == 0)
      {
        int const secondaryRes = m_StrFn.SecondaryCompare(m_StrFn.m_pData, key1, key2);
        if (bDeleteKey1)
          m_StrFn.Destroy(key1);
        return secondaryRes;
      }
      else
      {
        if (bDeleteKey1)
          m_StrFn.Destroy(key1);
        return primaryRes;
      }
    }

    bool operator () (uint32_t const id1, uint32_t const id2)
    {
      return Compare(id1, id2) < 0;
    }

  private:

    inline void KeysById(uint32_t const id1, uint32_t const id2,
                         sl::StrFn::Str const * & key1,
                         sl::StrFn::Str const * & key2,
                         bool & bDeleteKey1)
    {
      bDeleteKey1 = false;

      if (id1 == DIC_ID_MAGIC_EMPTY_VALUE)
        key1 = m_Special;
      else
      {
        bool found = false;
        sl::StrFn::Str const * & s = m_Cache.Find(id1, found);
        if (!found)
        {
          if (s != NULL)
            m_StrFn.Destroy(s);
          s = CreateKeyById(id1);
        }
        key1 = s;
      }

      if (id2 == DIC_ID_MAGIC_EMPTY_VALUE)
        key2 = m_Special;
      else
      {
        bool found = false;
        sl::StrFn::Str const * & s = m_Cache.Find(id2, found);
        if (!found)
        {
          if (s != NULL)
          {
            if (s == key1)
              bDeleteKey1 = true;
            else
              m_StrFn.Destroy(s);
          }
          s = CreateKeyById(id2);
        }
        key2 = s;
      }
    }

    inline sl::StrFn::Str const * CreateKeyById(uint32_t const id)
    {
      ASSERT_LESS(id, DIC_ID_MAGIC_EMPTY_VALUE, ());
      string keyStr;
      m_Dic.KeyById(id, keyStr);
      return m_StrFn.Create(&keyStr[0], keyStr.size());
    }

    sl::Dictionary const & m_Dic;
    sl::StrFn m_StrFn;
    sl::StrFn::Str const * m_Special;
    my::Cache<uint32_t, sl::StrFn::Str const *> m_Cache;
  };

  // Hack: global data, used for sorting with c-style timsort().
  static DictCollIdLess<true> * g_pSortedIndexDictCollIdLessForBuild = NULL;

  template <class T> struct LessRefProxy
  {
    T & m_F;
    explicit LessRefProxy(T & f) : m_F(f) {}
    inline bool operator () (SortedIndex::DicId a, SortedIndex::DicId b) const
    {
      return m_F(a.Value(), b.Value());
    }

    // Hack: used for sorting with c-style timsort().
    static int GlobalCompareForSort(void const * pVoidA, void const * pVoidB)
    {
      SortedIndex::DicId const * pA = static_cast<SortedIndex::DicId const *>(pVoidA);
      SortedIndex::DicId const * pB = static_cast<SortedIndex::DicId const *>(pVoidB);
      return g_pSortedIndexDictCollIdLessForBuild->Compare(pA->Value(), pB->Value());
    }
  };
  template <class T> LessRefProxy<T> MakeLessRefProxy(T & f) { return LessRefProxy<T>(f); }

} // namespace impl
} // namespace sl

sl::SortedIndex::Pos sl::SortedIndex::PrefixSearch(string const & prefix)
{
  impl::DictCollIdLess<false> compareLess(m_Dictionary, m_StrFn, prefix);
  return lower_bound(m_SortedVector.begin(), m_SortedVector.end(), DIC_ID_MAGIC_EMPTY_VALUE,
                     MakeLessRefProxy(compareLess))
         - m_SortedVector.begin();
}

void sl::SortedIndex::Build(sl::Dictionary const & dictionary,
                            StrFn const & strFn,
                            string const & indexPrefix)
{
  LOG(LINFO, ("Building sorted index."));

  // Initializing.
  vector<DicId> ids(dictionary.KeyCount());
  for (size_t i = 0; i < ids.size(); ++i)
    ids[i] = i;

  // Sorting.
  {
    sl::impl::DictCollIdLess<true> compareLess(dictionary, strFn);

    sl::impl::g_pSortedIndexDictCollIdLessForBuild = &compareLess;
    timsort(&ids[0], ids.size(), sizeof(ids[0]),
            &sl::impl::LessRefProxy<sl::impl::DictCollIdLess<true> >::GlobalCompareForSort);
    sl::impl::g_pSortedIndexDictCollIdLessForBuild = NULL;

    // sort() or stable_sort() are nice but slow.
    // sort()            150 milliseconds
    // stable_sort()      50 milliseconds
    // timsort()           9 milliseconds
    // stable_sort(ids.begin(), ids.end(), MakeLessRefProxy(compareLess));
  }

  FileWriter idxWriter(GetPlatform().WritablePathForFile(indexPrefix + ".idx").c_str());
  idxWriter.Write(&ids[0], ids.size() * sizeof(ids[0]));

  LOG(LINFO, ("Building sorted index done."));
}
