#pragma once
#include "../std/string.hpp"

class FeaturesVector;
class Writer;

namespace indexer
{

void BuildSearchIndex(FeaturesVector const & featuresVector, Writer & writer);
bool BuildSearchIndexFromDatFile(string const & datFile);

}  // namespace indexer
