#pragma once
#include "../std/string.hpp"


namespace indexer
{
  bool BuildSearchIndexFromDatFile(string const & fName, bool forceRebuild = false);
}
