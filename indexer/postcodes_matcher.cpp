#include "indexer/postcodes_matcher.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/transform_iterator.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

using namespace strings;

namespace search
{
namespace
{
// Top patterns for postcodes. See
// search/search_quality/clusterize_postcodes.lisp for details how
// these patterns were constructed.
char const * const g_patterns[] = {
    "aa nnnn",   "aa nnnnn",   "aaa nnnn",    "aan",      "aan naa",  "aana naa", "aann",
    "aann naa",  "aannaa",     "aannnaa",     "aannnn",   "an naa",   "ana naa",  "ana nan",
    "ananan",    "ann aann",   "ann naa",     "annnnaaa", "nn nnn",   "nnn",      "nnn nn",
    "nnn nnn",   "nnn nnnn",   "nnnn",        "nnnn aa",  "nnnn nnn", "nnnnaa",   "nnnnn",
    "nnnnn nnn", "nnnnn nnnn", "nnnnn nnnnn", "nnnnnn",   "nnnnnnn",  "nnnnnnnn", "〒nnn nnnn"};

UniChar SimplifyChar(UniChar const & c)
{
  if (IsASCIIDigit(c))
    return 'n';
  if (IsASCIILatin(c))
    return 'a';
  return c;
}

struct Node
{
  Node() : m_isLeaf(false) {}

  Node const * Move(UniChar c) const
  {
    for (auto const & p : m_moves)
    {
      if (p.first == c)
        return p.second.get();
    }
    return nullptr;
  }

  template <typename TIt>
  Node const * Move(TIt begin, TIt end) const
  {
    Node const * cur = this;
    for (; begin != end && cur; ++begin)
      cur = cur->Move(*begin);
    return cur;
  }

  Node & MakeMove(UniChar c)
  {
    for (auto const & p : m_moves)
    {
      if (p.first == c)
        return *p.second;
    }
    m_moves.emplace_back(c, make_unique<Node>());
    return *m_moves.back().second;
  }

  template <typename TIt>
  Node & MakeMove(TIt begin, TIt end)
  {
    Node * cur = this;
    for (; begin != end; ++begin)
      cur = &cur->MakeMove(*begin);
    return *cur;
  }

  buffer_vector<pair<UniChar, unique_ptr<Node>>, 2> m_moves;
  bool m_isLeaf;

  DISALLOW_COPY(Node);
};

// This class puts all strings from g_patterns to a trie with a low
// branching factor and matches queries against these patterns.
class PostcodesMatcher
{
public:
  PostcodesMatcher() : m_root(), m_maxNumTokensInPostcode(0)
  {
    search::Delimiters delimiters;
    for (auto const * pattern : g_patterns)
      AddString(MakeUniString(pattern), delimiters);
  }

  // Checks that given tokens match to at least one of postcodes
  // patterns.
  //
  // Complexity: O(total length of tokens in |slice|).
  bool HasString(StringSliceBase const & slice, bool handleAsPrefix) const
  {
    if (slice.Size() == 0)
      return m_root.m_isLeaf;

    Node const * cur = &m_root;
    for (size_t i = 0; i < slice.Size() && cur; ++i)
    {
      auto const & s = slice.Get(i);
      cur = cur->Move(make_transform_iterator(s.begin(), &SimplifyChar),
                      make_transform_iterator(s.end(), &SimplifyChar));
      if (cur && i + 1 < slice.Size())
        cur = cur->Move(' ');
    }

    if (!cur)
      return false;

    if (handleAsPrefix)
      return true;

    return cur->m_isLeaf;
  }

  inline size_t GetMaxNumTokensInPostcode() const { return m_maxNumTokensInPostcode; }

private:
  void AddString(UniString const & s, search::Delimiters & delimiters)
  {
    vector<UniString> tokens;
    SplitUniString(s, MakeBackInsertFunctor(tokens), delimiters);
    m_maxNumTokensInPostcode = max(m_maxNumTokensInPostcode, tokens.size());

    Node * cur = &m_root;
    for (size_t i = 0; i < tokens.size(); ++i)
    {
      cur = &cur->MakeMove(tokens[i].begin(), tokens[i].end());
      if (i + 1 != tokens.size())
          cur = &cur->MakeMove(' ');
    }
    cur->m_isLeaf = true;
  }

  Node m_root;

  size_t m_maxNumTokensInPostcode;

  DISALLOW_COPY(PostcodesMatcher);
};

PostcodesMatcher const & GetPostcodesMatcher()
{
  static PostcodesMatcher kMatcher;
  return kMatcher;
}
}  // namespace

bool LooksLikePostcode(StringSliceBase const & slice, bool handleAsPrefix)
{
  return GetPostcodesMatcher().HasString(slice, handleAsPrefix);
}

bool LooksLikePostcode(string const & s, bool handleAsPrefix)
{
  vector<UniString> tokens;
  bool const lastTokenIsPrefix =
      TokenizeStringAndCheckIfLastTokenIsPrefix(s, tokens, search::Delimiters());

  return LooksLikePostcode(NoPrefixStringSlice(tokens), handleAsPrefix && lastTokenIsPrefix);
}

size_t GetMaxNumTokensInPostcode() { return GetPostcodesMatcher().GetMaxNumTokensInPostcode(); }
}  // namespace search
