#include "map/user_mark_container.hpp"

#include "map/framework.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "base/scope_guard.hpp"
#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"

////////////////////////////////////////////////////////////////////////

namespace
{
//  class PinAnimation : public AnimPhaseChain
//  {
//  public:
//    PinAnimation(Framework & f)
//      : AnimPhaseChain(f, m_scale)
//      , m_scale(0.0)
//    {
//      InitDefaultPinAnim(this);
//    }

//    double GetScale() const
//    {
//      return m_scale;
//    }

//  private:
//    double m_scale;
//  };

  class FindMarkFunctor
  {
  public:
    FindMarkFunctor(UserMark ** mark, double & minD, m2::AnyRectD const & rect)
      : m_mark(mark)
      , m_minD(minD)
      , m_rect(rect)
    {
      m_globalCenter = rect.GlobalCenter();
    }

    void operator()(UserMark * mark)
    {
      m2::PointD const & org = mark->GetOrg();
      if (m_rect.IsPointInside(org))
      {
        double minDCandidate = m_globalCenter.SquareLength(org);
        if (minDCandidate < m_minD)
        {
          *m_mark = mark;
          m_minD = minDCandidate;
        }
      }
    }

    UserMark ** m_mark;
    double & m_minD;
    m2::AnyRectD const & m_rect;
    m2::PointD m_globalCenter;
  };

  df::TileKey CreateTileKey(UserMarkContainer const * cont)
  {
    switch (cont->GetType())
    {
    case UserMarkType::API_MARK: return df::GetApiTileKey();
    case UserMarkType::SEARCH_MARK: return df::GetSearchTileKey();
    case UserMarkType::BOOKMARK_MARK: return df::GetBookmarkTileKey(reinterpret_cast<size_t>(cont));
    default:
      ASSERT(false, ());
      break;
    }

    return df::TileKey();
  }

  size_t const VisibleFlag = 0;
  size_t const VisibleDirtyFlag = 1;
  size_t const DrawableFlag = 2;
  size_t const DrawableDirtyFlag = 3;
}

UserMarkContainer::UserMarkContainer(double layerDepth, UserMarkType type, Framework & fm)
  : m_framework(fm)
  , m_layerDepth(layerDepth)
  , m_type(type)
{
  m_flags.set();
}

UserMarkContainer::~UserMarkContainer()
{
  RequestController().Clear();
  ReleaseController();
}

UserMark const * UserMarkContainer::FindMarkInRect(m2::AnyRectD const & rect, double & d) const
{
  UserMark * mark = nullptr;
  if (IsVisible())
  {
    FindMarkFunctor f(&mark, d, rect);
    for (size_t i = 0; i < m_userMarks.size(); ++i)
    {
      if (rect.IsPointInside(m_userMarks[i]->GetOrg()))
        f(m_userMarks[i]);
    }
  }
  return mark;
}

namespace
{
unique_ptr<PoiMarkPoint> g_selectionUserMark;
unique_ptr<MyPositionMarkPoint> g_myPosition;
}

UserMarkDLCache::Key UserMarkContainer::GetDefaultKey() const
{
  return UserMarkDLCache::Key(GetTypeName(), graphics::EPosCenter, GetDepth());
}

void UserMarkContainer::InitStaticMarks(UserMarkContainer * container)
{
  if (g_selectionUserMark == NULL)
    g_selectionUserMark.reset(new PoiMarkPoint(container));

  if (g_myPosition == NULL)
    g_myPosition.reset(new MyPositionMarkPoint(container));
}

PoiMarkPoint * UserMarkContainer::UserMarkForPoi()
{
  ASSERT(g_selectionUserMark != NULL, ());
  return g_selectionUserMark.get();
}

MyPositionMarkPoint * UserMarkContainer::UserMarkForMyPostion()
{
  ASSERT(g_myPosition != NULL, ());
  return g_myPosition.get();
}

UserMarksController & UserMarkContainer::RequestController()
{
  BeginWrite();
  return *this;
}

void UserMarkContainer::ReleaseController()
{
  MY_SCOPE_GUARD(endWriteGuard, [this]{ EndWrite(); });
  dp::RefPointer<df::DrapeEngine> engine = m_framework.GetDrapeEngine();
  if (engine.IsNull())
    return;

  df::TileKey key = CreateTileKey(this);
  if (IsVisibleFlagDirty() || IsDrawableFlagDirty())
    engine->ChangeVisibilityUserMarksLayer(key, IsVisible() && IsDrawable());

  if (IsDirty())
  {
    if (GetUserMarkCount() == 0)
      engine->ClearUserMarksLayer(key);
    else
      engine->UpdateUserMarksLayer(key, this);
  }
}

size_t UserMarkContainer::GetCount() const
{
  return m_userMarks.size();
}

m2::PointD const & UserMarkContainer::GetPivot(size_t index) const
{
  return GetUserMark(index)->GetOrg();
}

float UserMarkContainer::GetDepth(size_t index) const
{
  UNUSED_VALUE(index);
  return m_layerDepth;
}

dp::Anchor UserMarkContainer::GetAnchor(size_t index) const
{
  UNUSED_VALUE(index);
  return dp::Center;
}

bool UserMarkContainer::IsVisible() const
{
  return m_flags[VisibleFlag];
}

bool UserMarkContainer::IsDrawable() const
{
  return m_flags[DrawableFlag];
}

UserMark * UserMarkContainer::CreateUserMark(m2::PointD const & ptOrg)
{
  SetDirty();
  m_userMarks.push_back(AllocateUserMark(ptOrg));
  return m_userMarks.back();
}

size_t UserMarkContainer::GetUserMarkCount() const
{
  return m_userMarks.size();
}

UserMark const * UserMarkContainer::GetUserMark(size_t index) const
{
  ASSERT_LESS(index, m_userMarks.size(), ());
  return m_userMarks[index];
}

UserMarkType UserMarkContainer::GetType() const
{
  return m_type;
}

UserMark * UserMarkContainer::GetUserMarkForEdit(size_t index)
{
  SetDirty();
  ASSERT_LESS(index, m_userMarks.size(), ());
  return m_userMarks[index];
}

void UserMarkContainer::Clear(size_t skipCount/* = 0*/)
{
  SetDirty();
  for (size_t i = skipCount; i < m_userMarks.size(); ++i)
    delete m_userMarks[i];

  if (skipCount < m_userMarks.size())
    m_userMarks.erase(m_userMarks.begin() + skipCount, m_userMarks.end());
}

void UserMarkContainer::SetIsDrawable(bool isDrawable)
{
  if (IsDrawable() != isDrawable)
  {
    m_flags[DrawableDirtyFlag] = true;
    m_flags[DrawableFlag] = isDrawable;
  }
}

void UserMarkContainer::SetIsVisible(bool isVisible)
{
  if (IsVisible() != isVisible)
  {
    m_flags[VisibleDirtyFlag] = true;
    m_flags[VisibleFlag] = isVisible;
  }
}

bool UserMarkContainer::IsVisibleFlagDirty()
{
  return m_flags[VisibleDirtyFlag];
}

bool UserMarkContainer::IsDrawableFlagDirty()
{
  return m_flags[DrawableDirtyFlag];
}

DebugUserMarkContainer::DebugUserMarkContainer(double layerDepth, Framework & framework)
  : UserMarkContainer(layerDepth, framework)
{
}

string DebugUserMarkContainer::GetTypeName() const
{
  // api-result.png is reused for debug markers
  return "api-result";
}

string DebugUserMarkContainer::GetActiveTypeName() const
{
  // api-result.png is reused for debug markers
  return "api-result";
}

UserMark * DebugUserMarkContainer::AllocateUserMark(const m2::PointD & ptOrg)
{
  return new DebugMarkPoint(ptOrg, this);
}

namespace
{

template <class T> void DeleteItem(vector<T> & v, size_t i)
{
  if (i < v.size())
  {
    delete v[i];
    v.erase(v.begin() + i);
  }
  else
  {
    LOG(LWARNING, ("Trying to delete non-existing item at index", i));
  }
}

}

void UserMarkContainer::DeleteUserMark(size_t index)
{
  SetDirty();
  ASSERT_LESS(index, m_userMarks.size(), ());
  if (index < m_userMarks.size())
  {
    delete m_userMarks[index];
    m_userMarks.erase(m_userMarks.begin() + index);
  }
  else
    LOG(LWARNING, ("Trying to delete non-existing item at index", index));
}

SearchUserMarkContainer::SearchUserMarkContainer(double layerDepth, Framework & framework)
  : UserMarkContainer(layerDepth, UserMarkType::SEARCH_MARK, framework)
{
}

string const & SearchUserMarkContainer::GetSymbolName(size_t index) const
{
  UNUSED_VALUE(index);
  static string s_symbol = "search-result";
  return s_symbol;
}

UserMark * SearchUserMarkContainer::AllocateUserMark(const m2::PointD & ptOrg)
{
  return new SearchMarkPoint(ptOrg, this);
}

ApiUserMarkContainer::ApiUserMarkContainer(double layerDepth, Framework & framework)
  : UserMarkContainer(layerDepth, UserMarkType::API_MARK, framework)
{
}

string const & ApiUserMarkContainer::GetSymbolName(size_t index) const
{
  UNUSED_VALUE(index);
  static string s_symbol = "api-result";
  return s_symbol;
}

UserMark * ApiUserMarkContainer::AllocateUserMark(const m2::PointD & ptOrg)
{
  return new ApiMarkPoint(ptOrg, this);
}
