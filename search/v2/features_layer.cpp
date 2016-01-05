#include "search/v2/features_layer.hpp"

#include "base/internal/message.hpp"

#include "std/sstream.hpp"

namespace search
{
namespace v2
{
FeaturesLayer::FeaturesLayer() { Clear(); }

void FeaturesLayer::Clear()
{
  m_sortedFeatures = nullptr;
  m_subQuery.clear();
  m_startToken = 0;
  m_endToken = 0;
  m_type = SearchModel::SEARCH_TYPE_COUNT;
  m_hasDelayedFeatures = false;
}

string DebugPrint(FeaturesLayer const & layer)
{
  ostringstream os;
  os << "FeaturesLayer [ size of m_sortedFeatures: "
     << (layer.m_sortedFeatures ? layer.m_sortedFeatures->size() : 0)
     << ", m_subQuery: " << layer.m_subQuery << ", m_startToken: " << layer.m_startToken
     << ", m_endToken: " << layer.m_endToken << ", m_type: " << DebugPrint(layer.m_type) << " ]";
  return os.str();
}
}  // namespace v2
}  // namespace search
