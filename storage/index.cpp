#include "storage/index.hpp"

#include "std/sstream.hpp"


namespace storage
{
  // GCC bug? Can't move initialization to hpp file (linker error).
  const int TIndex::INVALID = -1;

  string DebugPrint(TIndex const & r)
  {
    ostringstream out;
    out << "storage::TIndex(" << r.m_group << ", " << r.m_country << ", " << r.m_region << ")";
    return out.str();
  }
}
