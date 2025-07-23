#include "search/features_layer.hpp"

#include "base/internal/message.hpp"

#include <iostream>
#include <sstream>

using namespace std;

namespace search
{
FeaturesLayer::FeaturesLayer()
{
  Clear();
}

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
  os << "FeaturesLayer [size of m_sortedFeatures: " << (layer.m_sortedFeatures ? layer.m_sortedFeatures->size() : 0)
     << ", subquery: " << DebugPrint(layer.m_subQuery) << ", tokenRange: " << DebugPrint(layer.m_tokenRange)
     << ", type: " << DebugPrint(layer.m_type) << ", lastTokenIsPrefix: " << layer.m_lastTokenIsPrefix << "]";
  return os.str();
}
}  // namespace search
