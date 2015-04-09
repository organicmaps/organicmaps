#include "indexer/feature_decl.hpp"

#include "std/sstream.hpp"


string DebugPrint(FeatureID const & id)
{
  ostringstream ss;
  ss << "{ " << id.m_mwm << ", " << id.m_offset << " }";
  return ss.str();
}
