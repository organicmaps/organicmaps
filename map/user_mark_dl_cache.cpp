#include "map/user_mark_dl_cache.hpp"

#include "base/stl_add.hpp"

UserMarkDLCache::UserMarkDLCache()
{
}

UserMarkDLCache::~UserMarkDLCache()
{
}

///@TODO UVR
//graphics::DisplayList * UserMarkDLCache::FindUserMark(UserMarkDLCache::Key const & key)
//{
//  node_t node = m_dls.find(key);
//  if (node != m_dls.end())
//    return node->second;

//  return CreateDL(key);
//}

namespace
{
///@TODO UVR
//  m2::RectD CalcCoords(double const & halfSizeX, double const & halfSizeY, graphics::EPosition anchor)
//  {
//    m2::RectD result(-halfSizeX, -halfSizeY, halfSizeX, halfSizeY);

//    if (anchor & graphics::EPosAbove)
//      result.Offset(0.0, -halfSizeY);
//    else if (anchor & graphics::EPosUnder)
//      result.Offset(0.0, halfSizeY);

//    if (anchor & graphics::EPosLeft)
//      result.Offset(halfSizeX, 0.0);
//    else if (anchor & graphics::EPosRight)
//      result.Offset(-halfSizeX, 0.0);

//    return result;
//  }
}

///@TODO UVR
//graphics::DisplayList * UserMarkDLCache::CreateDL(UserMarkDLCache::Key const & key)
//{
//  using namespace graphics;

//  graphics::DisplayList * dl = m_cacheScreen->createDisplayList();
//  m_cacheScreen->beginFrame();
//  m_cacheScreen->setDisplayList(dl);

//  Icon::Info infoKey(key.m_name);
//  Resource const * res = m_cacheScreen->fromID(m_cacheScreen->findInfo(infoKey));
//  shared_ptr<gl::BaseTexture> texture = m_cacheScreen->pipeline(res->m_pipelineID).texture();

//  m2::RectU texRect = res->m_texRect;
//  m2::RectD coord = CalcCoords(texRect.SizeX() / 2.0, texRect.SizeY() / 2.0, key.m_anchor);

//  m2::PointD coords[] =
//  {
//    coord.LeftBottom(),
//    coord.LeftTop(),
//    coord.RightBottom(),
//    coord.RightTop()
//  };
//  m2::PointF normal(0.0, 0.0);

//  m2::PointF texCoords[] =
//  {
//    texture->mapPixel(m2::PointF(texRect.minX(), texRect.minY())),
//    texture->mapPixel(m2::PointF(texRect.minX(), texRect.maxY())),
//    texture->mapPixel(m2::PointF(texRect.maxX(), texRect.minY())),
//    texture->mapPixel(m2::PointF(texRect.maxX(), texRect.maxY()))
//  };

//  m_cacheScreen->addTexturedStripStrided(coords, sizeof(m2::PointD),
//                                       &normal, 0,
//                                       texCoords, sizeof(m2::PointF),
//                                       4, key.m_depthLayer, res->m_pipelineID);

//  m_cacheScreen->setDisplayList(NULL);
//  m_cacheScreen->endFrame();

//  m_dls.insert(make_pair(key, dl));

//  return dl;
//}
