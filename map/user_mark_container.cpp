#include "map/framework.hpp"
#include "map/user_mark_container.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "base/scope_guard.hpp"
#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"

namespace
{
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
      m2::PointD const & org = mark->GetPivot();
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
    case UserMarkType::API_MARK:
      return df::GetApiTileKey();
    case UserMarkType::SEARCH_MARK:
      return df::GetSearchTileKey();
    case UserMarkType::BOOKMARK_MARK:
      return df::GetBookmarkTileKey(reinterpret_cast<size_t>(cont));
    case UserMarkType::DEBUG_MARK:
      return df::GetDebugTileKey();
    }

    ASSERT(false, ());
    return df::TileKey();
  }

  size_t const VisibleFlag = 0;
  size_t const DrawableFlag = 1;
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
      if (rect.IsPointInside(m_userMarks[i]->GetPivot()))
         f(m_userMarks[i].get());
    }
  }
  return mark;
}

namespace
{
unique_ptr<PoiMarkPoint> g_selectionUserMark;
unique_ptr<MyPositionMarkPoint> g_myPosition;
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
  ref_ptr<df::DrapeEngine> engine = m_framework.GetDrapeEngine();
  if (engine == nullptr)
    return;

  df::TileKey key = CreateTileKey(this);
  engine->ChangeVisibilityUserMarksLayer(key, IsVisible() && IsDrawable());

  if (IsDirty())
  {
    if (GetUserPointCount() == 0 && GetUserLineCount() == 0)
      engine->ClearUserMarksLayer(key);
    else
      engine->UpdateUserMarksLayer(key, this);
  }
}

size_t UserMarkContainer::GetUserPointCount() const
{
  return m_userMarks.size();
}

df::UserPointMark const * UserMarkContainer::GetUserPointMark(size_t index) const
{
  return GetUserMark(index);
}

size_t UserMarkContainer::GetUserLineCount() const
{
  return 0;
}

df::UserLineMark const * UserMarkContainer::GetUserLineMark(size_t index) const
{
  UNUSED_VALUE(index);
  ASSERT(false, ());
  return nullptr;
}

float UserMarkContainer::GetPointDepth() const
{
  return m_layerDepth;
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
  // Push front an user mark.
  SetDirty();
  m_userMarks.push_front(unique_ptr<UserMark>(AllocateUserMark(ptOrg)));
  return m_userMarks.front().get();
}

size_t UserMarkContainer::GetUserMarkCount() const
{
  return GetUserPointCount();
}

UserMark const * UserMarkContainer::GetUserMark(size_t index) const
{
  ASSERT_LESS(index, m_userMarks.size(), ());
  return m_userMarks[index].get();
}

UserMarkType UserMarkContainer::GetType() const
{
  return m_type;
}

UserMark * UserMarkContainer::GetUserMarkForEdit(size_t index)
{
  SetDirty();
  ASSERT_LESS(index, m_userMarks.size(), ());
  return m_userMarks[index].get();
}

void UserMarkContainer::Clear(size_t skipCount/* = 0*/)
{
  SetDirty();
  if (skipCount < m_userMarks.size())
    m_userMarks.erase(m_userMarks.begin(), m_userMarks.end() - skipCount);
}

void UserMarkContainer::SetIsDrawable(bool isDrawable)
{
  if (IsDrawable() != isDrawable)
    m_flags[DrawableFlag] = isDrawable;
}

void UserMarkContainer::SetIsVisible(bool isVisible)
{
  if (IsVisible() != isVisible)
    m_flags[VisibleFlag] = isVisible;
}

void UserMarkContainer::Update()
{
  SetDirty();
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
    m_userMarks.erase(m_userMarks.begin() + index);
  else
    LOG(LWARNING, ("Trying to delete non-existing item at index", index));
}

SearchUserMarkContainer::SearchUserMarkContainer(double layerDepth, Framework & framework)
  : UserMarkContainer(layerDepth, UserMarkType::SEARCH_MARK, framework)
{
}

UserMark * SearchUserMarkContainer::AllocateUserMark(const m2::PointD & ptOrg)
{
  return new SearchMarkPoint(ptOrg, this);
}

DebugUserMarkContainer::DebugUserMarkContainer(double layerDepth, Framework & framework)
  : UserMarkContainer(layerDepth, UserMarkType::DEBUG_MARK, framework)
{
}

UserMark * DebugUserMarkContainer::AllocateUserMark(const m2::PointD & ptOrg)
{
  return new DebugMarkPoint(ptOrg, this);
}
