#include "search/features_layer.hpp"

#include "base/internal/message.hpp"

#include "std/sstream.hpp"

namespace search
{
FeaturesLayer::FeaturesLayer() { Clear(); }

void FeaturesLayer::Clear()
{
  m_sortedFeatures = nullptr;
  m_subQuery.clear();
  m_tokenRange.Clear();
  m_type = Model::TYPE_COUNT;
  m_hasDelayedFeatures = false;
  m_lastTokenIsPrefix = false;
}

string DebugPrint(FeaturesLayer const & layer)
{
  ostringstream os;
  os << "FeaturesLayer [ size of m_sortedFeatures: "
     << (layer.m_sortedFeatures ? layer.m_sortedFeatures->size() : 0)
     << ", m_subQuery: " << DebugPrint(layer.m_subQuery)
     << ", m_tokenRange: " << DebugPrint(layer.m_tokenRange)
     << ", m_type: " << DebugPrint(layer.m_type)
     << ", m_lastTokenIsPrefix: " << layer.m_lastTokenIsPrefix << " ]";
  return os.str();
}
}  // namespace search
