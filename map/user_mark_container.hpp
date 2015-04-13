#pragma once

#include "map/events.hpp"
#include "map/user_mark.hpp"

#include "drape_frontend/user_marks_provider.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/deque.hpp"
#include "std/bitset.hpp"

#include "std/noncopyable.hpp"
#include "std/unique_ptr.hpp"

class Framework;

enum class UserMarkType
{
  SEARCH_MARK,
  API_MARK,
  DEBUG_MARK,
  BOOKMARK_MARK
};

class UserMarksController
{
public:
  virtual size_t GetUserMarkCount() const = 0;
  virtual UserMarkType GetType() const = 0;
  virtual void SetIsDrawable(bool isDrawable) = 0;
  virtual void SetIsVisible(bool isVisible) = 0;

  virtual UserMark * CreateUserMark(m2::PointD const & ptOrg) = 0;
  virtual UserMark const * GetUserMark(size_t index) const = 0;
  virtual UserMark * GetUserMarkForEdit(size_t index) = 0;
  virtual void DeleteUserMark(size_t index) = 0;
  virtual void Clear(size_t skipCount = 0) = 0;
};

class UserMarkContainer : public df::UserMarksProvider
                        , protected UserMarksController
                        , private noncopyable
{
public:
  using TUserMarksList = deque<unique_ptr<UserMark>>;

  UserMarkContainer(double layerDepth, UserMarkType type, Framework & fm);
  virtual ~UserMarkContainer();

  // If not found mark on rect result is nullptr
  // If mark is found in "d" return distance from rect center
  // In multiple select choose mark with min(d)
  UserMark const * FindMarkInRect(m2::AnyRectD const & rect, double & d) const;

  static void InitStaticMarks(UserMarkContainer * container);
  static PoiMarkPoint * UserMarkForPoi();
  static MyPositionMarkPoint * UserMarkForMyPostion();

  /// never save reference on UserMarksController
  UserMarksController & RequestController();
  void ReleaseController();

  ////////////////////////////////////////////////////////////
  /// Render info
  virtual size_t GetUserPointCount() const;
  virtual df::UserPointMark const * GetUserPointMark(size_t index) const;

  virtual size_t GetUserLineCount() const;
  virtual df::UserLineMark const * GetUserLineMark(size_t index) const;
  ////////////////////////////////////////////////////////////

  float GetPointDepth() const;

  bool IsVisible() const;
  bool IsDrawable() const override;
  size_t GetUserMarkCount() const override;
  UserMark const * GetUserMark(size_t index) const override;
  UserMarkType GetType() const override final;

protected:
  /// UserMarksController implementation
  virtual UserMark * CreateUserMark(m2::PointD const & ptOrg);
  virtual UserMark * GetUserMarkForEdit(size_t index);
  virtual void DeleteUserMark(size_t index);
  virtual void Clear(size_t skipCount = 0);
  virtual void SetIsDrawable(bool isDrawable);
  virtual void SetIsVisible(bool isVisible);

protected:
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg) = 0;

protected:
  Framework & m_framework;

private:
  bool IsVisibleFlagDirty();
  bool IsDrawableFlagDirty();

private:
  bitset<4> m_flags;
  double m_layerDepth;
  TUserMarksList m_userMarks;
  UserMarkType m_type;
};

class SearchUserMarkContainer : public UserMarkContainer
{
public:
  SearchUserMarkContainer(double layerDepth, Framework & framework);

protected:
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);
};

class ApiUserMarkContainer : public UserMarkContainer
{
public:
  ApiUserMarkContainer(double layerDepth, Framework & framework);

protected:
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);
};

class DebugUserMarkContainer : public UserMarkContainer
{
public:
  DebugUserMarkContainer(double layerDepth, Framework & framework);

  virtual Type GetType() const { return DEBUG_MARK; }

  virtual string GetActiveTypeName() const;
protected:
  virtual string GetTypeName() const;
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);
};
