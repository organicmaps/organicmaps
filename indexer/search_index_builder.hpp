#pragma once
#include "../std/string.hpp"

class FeaturesVector;
class Writer;

namespace indexer
{
  void BuildSearchIndex(FeaturesVector const & featuresVector, Writer & writer,
                        string const & tmpFilePath);
  bool BuildSearchIndexFromDatFile(string const & fName);
}  // namespace indexer
