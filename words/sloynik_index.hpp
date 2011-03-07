#pragma once
#include "common.hpp"
#include "dictionary.hpp"
#include "../coding/polymorph_reader.hpp"
#include "../coding/dd_vector.hpp"
#include "../coding/file_reader.hpp"
#include "../base/base.hpp"
#include "../std/function.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

class Reader;
class Writer;

namespace sl
{
  namespace impl
  { // needed for friend declaration
    template <class T> struct LessRefProxy;
  }

// Sorted index of keys.
// Two locale-aware comparators are used: primary and secondary.
// Secondary comporator is only used to compare keys, which are equivalent
// according to primary comparator.
// For search only primary comporator is used.
// Do determine the order of keys to display, primary + secondary comporators are used.
// To collect articles for the same key, primary + secondary comporators are used.
class SortedIndex
{
public:
  typedef uint32_t Pos;

  SortedIndex(Dictionary const & dictionary, Reader const * pIndexReader, StrFn const & strFn);

  // Id of the smallest key that is equal or grater that prefix.
  Pos PrefixSearch(string const & prefix);

  Dictionary::Id KeyIdByPos(Pos pos) const;

  Dictionary const & GetDictionary() const;

  // Build index.
  static void Build(Dictionary const & dictionary,
                    StrFn const & strFn,
                    string const & indexPathPrefix);
private:

#pragma pack(push, 1)
  class DicId
  {
  public:
    DicId() : m_Lo(0), m_Mi(0), m_Hi(0) {}
    DicId(uint32_t x) : m_Lo(x & 0xFF), m_Mi((x >> 8) & 0xFF), m_Hi((x >> 16) & 0xFF)
    {
      ASSERT_LESS_OR_EQUAL(x, 0xFFFFFF, ());
    }

    uint32_t Value() const
    {
      return (uint32_t(m_Hi) << 16) | (uint32_t(m_Mi) << 8) | m_Lo;
    }

  private:
    uint8_t m_Lo, m_Mi, m_Hi;
  };
#pragma pack(pop)
  template <class T> friend struct impl::LessRefProxy;

  typedef DDVector<DicId, PolymorphReader> DDVectorType;

  Dictionary const & m_Dictionary;  // Dictionary.
  StrFn m_StrFn;                    // Primary and secondary comparison functions.
  DDVectorType m_SortedVector;      // Sorted array of key ids.
};


}
