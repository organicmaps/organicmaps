#pragma once
#include "compact_tree_builder.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../std/algorithm.hpp"
#include "../std/vector.hpp"
#include "../base/start_mem_debug.hpp"

// Build compact trie given a sorted sequence on strings.
// If writeChars == false, characters will not be written
template <class TChar, class TSink, class TIter>
void BuildMMCompactTrie(TSink & sink, TIter const beg, TIter const end, bool writeChars = true)
{
  size_t maxLen = 0;
  size_t size = 0;
  for (TIter it = beg; it != end; ++it, ++size)
    maxLen = max(maxLen, it->size());
  vector<vector<bool> > isParent(maxLen + 1);
  vector<vector<bool> > isFirstChild(maxLen + 1);
  vector<vector<bool> > hasData(maxLen + 1);
  vector<vector<TChar> > chars(maxLen);
  isParent[0].push_back(true);
  isFirstChild[0].push_back(false);
  hasData[0].push_back(false);
  TIter prev;
  size_t word = 0;
  for (TIter it = beg; it != end; ++word, prev = it++)
  {
    CHECK_NOT_EQUAL(it->size(), 0U, ());
    size_t commonLen = 0;
    bool nextIsExtensionOfPrev = true;
    if (it != beg)
    {
      while (commonLen < it->size() && commonLen < prev->size() &&
             (*it)[commonLen] == (*prev)[commonLen])
        ++commonLen;
      // Next string should be strictly greater than previous.
      CHECK(commonLen != it->size(),
            (commonLen, it->size(), prev->size(), word));
      CHECK(commonLen == prev->size() || (*prev)[commonLen] != (*it)[commonLen],
            (commonLen, it->size(), prev->size(), word));
      nextIsExtensionOfPrev = (commonLen == prev->size());
    }
    isParent[commonLen].back() = true;
    size_t last = it->size() - 1;
    for (size_t i = commonLen; i <= last; ++i)
    {
      isParent[i+1].push_back(i != last);
      isFirstChild[i+1].push_back(i != commonLen || nextIsExtensionOfPrev);
      hasData[i+1].push_back(i == last);
      if (writeChars)
        chars[i].push_back((*it)[i]);
    }
  }

  vector<bool> isParentComined, isFirstChildCombined, parentHasDataCombined;
  isParentComined.reserve(size);
  isFirstChildCombined.reserve(size);
  parentHasDataCombined.reserve(size);
  for (vector<vector<bool> >::const_iterator i = isParent.begin(); i != isParent.end(); ++i)
    isParentComined.insert(isParentComined.end(), i->begin(), i->end());
  for (vector<vector<bool> >::const_iterator i = isFirstChild.begin(); i != isFirstChild.end(); ++i)
    isFirstChildCombined.insert(isFirstChildCombined.end(), i->begin(), i->end());
  for (size_t i = 0; i < isParent.size(); ++i)
  {
    for (size_t j = 0; j < isParent[i].size(); ++j)
    {
      if (isParent[i][j])
      {
        parentHasDataCombined.push_back(hasData[i][j]);
      }
    }
  }

  BuildCompactTreeWithData(sink, isParentComined.begin(), isParentComined.end(),
                           isFirstChildCombined.begin(), isFirstChildCombined.end(),
                           parentHasDataCombined.begin(), parentHasDataCombined.end());
  size_t numChars = 0;
  if (writeChars)
  {
    for (typename vector<vector<TChar> >::const_iterator i = chars.begin(); i != chars.end(); ++i)
    {
      for (typename vector<TChar>::const_iterator it = i->begin(); it != i->end(); ++it)
      {
        WriteToSink(sink, TChar(*it));
        ++numChars;
      }
    }
  }

  size_t padding = (4 - (numChars * sizeof(TChar)) % 4) % 4;
  for (size_t i = 0; i < padding; ++i)
  {
    WriteToSink(sink, uint8_t(0));
  }
}

#include "../base/stop_mem_debug.hpp"
