#pragma once
#include "search_trie.hpp"

class FeaturesVector;

namespace search
{

namespace impl { class Query; }

void MatchAgainstTrie(impl::Query & query,
                      TrieIterator & trieRoot,
                      FeaturesVector const & featuresVector);

}  // namespace search
