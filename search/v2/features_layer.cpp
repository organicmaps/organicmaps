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
  m_sortedFeatures.clear();
  m_startToken = 0;
  m_endToken = 0;
  m_type = SearchModel::SEARCH_TYPE_COUNT;
}

string DebugPrint(FeaturesLayer const & layer)
{
  ostringstream os;
  os << "FeaturesLayer [ m_sortedFeatures: " << ::DebugPrint(layer.m_sortedFeatures)
     << ", m_startToken: " << layer.m_startToken << ", m_endToken: " << layer.m_endToken
     << ", m_type: " << DebugPrint(layer.m_type) << " ]";
  return os.str();
}
}  // namespace v2
}  // namespace search
