#include "indexer/feature_decl.hpp"

#include "std/sstream.hpp"


string DebugPrint(FeatureID const & id)
{
  ostringstream ss;
  ss << "{ " << DebugPrint(id.m_mwmId) << ", " << id.m_ind << " }";
  return ss.str();
}
