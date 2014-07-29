#include "BookMarkUtils.hpp"
#include "Utils.hpp"
#include "Constants.hpp"
#include "AppResourceId.h"
#include "../../../map/framework.hpp"
#include "../../../map/user_mark.hpp"
#include "../../../platform/tizen_utils.hpp"

using namespace Tizen::Base;
using namespace consts;

namespace bookmark
{

search::AddressInfo const GetAdressInfo(UserMark const * pUserMark)
{
  if (pUserMark)
  {
    switch (pUserMark->GetMarkType())
    {
      case UserMark::POI:
      {
        PoiMarkPoint const * pPOI = static_cast<PoiMarkPoint const *>(pUserMark);
        return pPOI->GetInfo();
      }
      break;
      case UserMark::SEARCH:
      {
        SearchMarkPoint const * pSearch = static_cast<SearchMarkPoint const *>(pUserMark);
        return pSearch->GetInfo();
      }
      break;
      case UserMark::BOOKMARK:
      {
        search::AddressInfo addressInfo;
        GetFramework()->GetAddressInfoForGlobalPoint(pUserMark->GetOrg(), addressInfo);
        return addressInfo;
      }
      break;
      default:
        break;
    }
  }
  return search::AddressInfo();
}

String GetMarkName(UserMark const * pUserMark)
{
  if (!pUserMark)
    return "";
  if (pUserMark->GetMarkType() == UserMark::BOOKMARK)
  {
    Bookmark const * pSearch = static_cast<Bookmark const *>(pUserMark);
    return pSearch->GetName().c_str();
  }
  else
  {
    return GetAdressInfo(pUserMark).GetPinName().c_str();
  }
}

String GetMarkType(UserMark const * pUserMark)
{
  return GetFeature(GetAdressInfo(pUserMark).GetPinType().c_str());
}

String GetMarkCountry(UserMark const * pUserMark)
{
  return GetAdressInfo(pUserMark).FormatAddress().c_str();
}

Tizen::Base::String GetDistance(UserMark const * pUserMark)
{
  if (!pUserMark)
    return "";
  double lat, lon;
  GetFramework()->GetCurrentPosition(lat, lon);
  double north = 0;
  string dist;
  double azimut;
  m2::PointD pt = pUserMark->GetOrg();
  GetFramework()->GetDistanceAndAzimut(pt, lat, lon, north, dist, azimut);
  return dist.c_str();
}

double GetAzimuth(UserMark const * pUserMark, double north)
{
  if (!pUserMark)
    return 0;
  double lat, lon;
  GetFramework()->GetCurrentPosition(lat, lon);
  string dist;
  double azimut;
  m2::PointD const pt = pUserMark->GetOrg();
  GetFramework()->GetDistanceAndAzimut(pt, lat, lon, north, dist, azimut);
  return azimut;
}

bool IsBookMark(UserMark const * pUserMark)
{
  if (!pUserMark)
    return false;
  return pUserMark->GetMarkType() == UserMark::BOOKMARK;
}

string fromEColorTostring(EColor color)
{
  switch(color)
  {
    case CLR_RED: return BM_COLOR_RED;
    case CLR_BLUE: return BM_COLOR_BLUE;
    case CLR_BROWN: return BM_COLOR_BROWN;
    case CLR_GREEN: return BM_COLOR_GREEN;
    case CLR_ORANGE: return BM_COLOR_ORANGE;
    case CLR_PINK: return BM_COLOR_PINK;
    case CLR_PURPLE: return BM_COLOR_PURPLE;
    case CLR_YELLOW: return BM_COLOR_YELLOW;
  }
  return BM_COLOR_RED;
}

EColor fromstringToColor(string const & sColor)
{

  if (sColor == BM_COLOR_RED)
    return CLR_RED;
  if (sColor == BM_COLOR_BLUE)
    return CLR_BLUE;
  if (sColor == BM_COLOR_BROWN)
    return CLR_BROWN;
  if (sColor == BM_COLOR_GREEN)
    return CLR_GREEN;
  if (sColor == BM_COLOR_ORANGE)
    return CLR_ORANGE;
  if (sColor == BM_COLOR_PINK)
    return CLR_PINK;
  if (sColor == BM_COLOR_PURPLE)
    return CLR_PURPLE;
  if (sColor == BM_COLOR_YELLOW)
    return CLR_YELLOW;

  return CLR_RED;
}

const wchar_t * GetColorBM(EColor color)
{
  switch(color)
  {
    case CLR_RED: return IDB_COLOR_RED;
    case CLR_BLUE: return IDB_COLOR_BLUE;
    case CLR_BROWN: return IDB_COLOR_BROWN;
    case CLR_GREEN: return IDB_COLOR_GREEN;
    case CLR_ORANGE: return IDB_COLOR_ORANGE;
    case CLR_PINK: return IDB_COLOR_PINK;
    case CLR_PURPLE: return IDB_COLOR_PURPLE;
    case CLR_YELLOW: return IDB_COLOR_YELLOW;
  }
  return IDB_COLOR_RED;
}

const wchar_t * GetColorSelecteBM(EColor color)
{
  switch(color)
  {
    case CLR_RED: return IDB_COLOR_SELECT_RED;
    case CLR_BLUE: return IDB_COLOR_SELECT_BLUE;
    case CLR_BROWN: return IDB_COLOR_SELECT_BROWN;
    case CLR_GREEN: return IDB_COLOR_SELECT_GREEN;
    case CLR_ORANGE: return IDB_COLOR_SELECT_ORANGE;
    case CLR_PINK: return IDB_COLOR_SELECT_PINK;
    case CLR_PURPLE: return IDB_COLOR_SELECT_PURPLE;
    case CLR_YELLOW: return IDB_COLOR_SELECT_YELLOW;
  }
  return IDB_COLOR_SELECT_RED;
}

const wchar_t * GetColorPPBM(EColor color)
{
  switch(color)
  {
    case CLR_RED: return IDB_COLOR_PP_RED;
    case CLR_BLUE: return IDB_COLOR_PP_BLUE;
    case CLR_BROWN: return IDB_COLOR_PP_BROWN;
    case CLR_GREEN: return IDB_COLOR_PP_GREEN;
    case CLR_ORANGE: return IDB_COLOR_PP_ORANGE;
    case CLR_PINK: return IDB_COLOR_PP_PINK;
    case CLR_PURPLE: return IDB_COLOR_PP_PURPLE;
    case CLR_YELLOW: return IDB_COLOR_PP_YELLOW;
  }
  return IDB_COLOR_PP_RED;
}

BookMarkManager & GetBMManager()
{
  return BookMarkManager::GetInstance();
}

BookMarkManager & BookMarkManager::GetInstance()
{
  static BookMarkManager instance;
  return instance;
}

BookMarkManager::BookMarkManager()
{
}

void BookMarkManager::ActivateBookMark(UserMarkCopy * pCopy)
{
  m_pCurBookMarkCopy.reset(pCopy);
  if (pCopy)
    GetFramework()->ActivateUserMark(pCopy->GetUserMark());
  else
    GetFramework()->ActivateUserMark(0);
}

void BookMarkManager::RemoveCurBookMark()
{
  Bookmark const * pBM = GetCurBookMark();
  if (!pBM)
    return;
  m2::PointD const ptOrg = pBM->GetOrg();
  ::Framework * pFramework = GetFramework();
  BookmarkAndCategory const & bookmarkAndCategory = pFramework->FindBookmark(pBM);
  BookmarkCategory * category = pFramework->GetBmCategory(bookmarkAndCategory.first);
  if (category)
  {
    category->DeleteBookmark(bookmarkAndCategory.second);
    category->SaveToKMLFile();
  }
  pFramework->Invalidate();
  ActivateBookMark(pFramework->GetAddressMark(ptOrg)->Copy());
}

void BookMarkManager::DeleteBookMark(size_t category, size_t index)
{
  ::Framework * pFramework = GetFramework();
  BookmarkCategory * pCategory = pFramework->GetBmCategory(category);
  if (pCategory)
  {
    pCategory->DeleteBookmark(index);
    pCategory->SaveToKMLFile();
  }
  pFramework->Invalidate();
  ActivateBookMark(0);
}
void BookMarkManager::ShowBookMark(size_t category, size_t index)
{
  ::Framework * pFramework = GetFramework();
  BookmarkCategory * pCategory = pFramework->GetBmCategory(category);
  ActivateBookMark(pCategory->GetBookmark(index)->Copy());
  pFramework->ShowBookmark(BookmarkAndCategory(category, index));
}

Bookmark const * BookMarkManager::GetBookMark(size_t category, size_t index)
{
  ::Framework * pFramework = GetFramework();
  BookmarkCategory * pCategory = pFramework->GetBmCategory(category);
  return pCategory->GetBookmark(index);
}

void BookMarkManager::AddCurMarkToBookMarks()
{
  if (!m_pCurBookMarkCopy)
    return;
  UserMark const * pUserMark = m_pCurBookMarkCopy->GetUserMark();
  if (!pUserMark)
    return;
  ::Framework * pFramework = GetFramework();

  size_t const categoryIndex = pFramework->LastEditedBMCategory();
  BookmarkData data(FromTizenString(GetMarkName(pUserMark)), pFramework->LastEditedBMType());
  m2::PointD const ptOrg = pUserMark->GetOrg();
  int i = pFramework->AddBookmark(categoryIndex, ptOrg, data);
  pFramework->GetBmCategory(categoryIndex)->SaveToKMLFile();
  pFramework->Invalidate();
  ActivateBookMark(pFramework->GetBmCategory(categoryIndex)->GetBookmark(i)->Copy());
}

UserMark const * BookMarkManager::GetCurMark() const
{
  if (!m_pCurBookMarkCopy)
    return 0;
  return m_pCurBookMarkCopy->GetUserMark();
}

Bookmark const * BookMarkManager::GetCurBookMark() const
{
  UserMark const * pMark = GetCurMark();
  if (!pMark)
    return 0;
  return dynamic_cast<Bookmark const *>(pMark);
}

String BookMarkManager::GetBookMarkMessage() const
{
  Bookmark const * pBM = GetCurBookMark();
  if (!pBM)
    return "";
  return pBM->GetDescription().c_str();
}

void BookMarkManager::SetBookMarkMessage(Tizen::Base::String const & s)
{
  Bookmark const * pBM = GetCurBookMark();
  if (pBM)
  {
    Framework * pFW = GetFramework();
    BookmarkAndCategory bmAndCat = pFW->FindBookmark(pBM);
    BookmarkData data = pBM->GetData();
    data.SetDescription(FromTizenString(s));
    pFW->ReplaceBookmark(bmAndCat.first, bmAndCat.second, data);
    pFW->GetBmCategory(bmAndCat.first)->SaveToKMLFile();
  }
}

int BookMarkManager::GetCategoriesCount() const
{
  return GetFramework()->GetBmCategoriesCount();
}

Tizen::Base::String BookMarkManager::GetCategoryName(int const index) const
{
  if (index > GetCategoriesCount())
    return "";
  if (index < 0)
    return "";
  BookmarkCategory * pCategory = GetFramework()->GetBmCategory(index);
  return pCategory->GetName().c_str();
}

void BookMarkManager::SetCategoryName(int const index, Tizen::Base::String const & sName) const
{
  if (index > GetCategoriesCount())
    return;
  if (index < 0)
    return;
  BookmarkCategory * pCategory = GetFramework()->GetBmCategory(index);
  pCategory->SetName(FromTizenString(sName));
  pCategory->SaveToKMLFile();
}

Tizen::Base::String BookMarkManager::GetCurrentCategoryName() const
{
  int const curCat = GetCurrentCategory();
  if (curCat < 0)
    return "";
  return GetCategoryName(curCat);
}

int BookMarkManager::AddCategory(Tizen::Base::String const & sName) const
{
  if (!sName.IsEmpty())
  {
    Framework * pFW = GetFramework();
    int i = pFW->AddCategory(FromTizenString(sName));
    pFW->GetBmCategory(i)->SaveToKMLFile();
    return i;
  }
  return -1;
}

int BookMarkManager::GetCurrentCategory() const
{
  Bookmark const * pBM = GetCurBookMark();
  if (pBM)
  {
    Framework * pFW = GetFramework();
    BookmarkAndCategory bmAndCat = pFW->FindBookmark(pBM);
    return bmAndCat.first;
  }
  return -1;
}

void BookMarkManager::SetNewCurBookMarkCategory(int const nNewCategory)
{
  if (nNewCategory >= GetCategoriesCount())
    return;
  Bookmark const * pBM = GetCurBookMark();
  if (!pBM)
    return;

  Framework * pFW = GetFramework();
  BookmarkAndCategory bmAndCat = pFW->FindBookmark(pBM);
  if (nNewCategory == bmAndCat.first)
    return;
  int newIndex = pFW->MoveBookmark(bmAndCat.second, bmAndCat.first, nNewCategory);
  pFW->GetBmCategory(bmAndCat.first)->SaveToKMLFile();
  pFW->GetBmCategory(nNewCategory)->SaveToKMLFile();

  Bookmark const * bookmark = pFW->GetBmCategory(nNewCategory)->GetBookmark(newIndex);
  m_pCurBookMarkCopy.reset(bookmark->Copy());
}

void BookMarkManager::SetCurBookMarkColor(EColor const color)
{
  Bookmark const * pBM = GetCurBookMark();
  if (pBM)
  {
    Framework * pFW = GetFramework();
    BookmarkAndCategory bmAndCat = pFW->FindBookmark(pBM);
    BookmarkData data = pBM->GetData();
    data.SetType(fromEColorTostring(color));
    pFW->ReplaceBookmark(bmAndCat.first, bmAndCat.second, data);
    pFW->GetBmCategory(bmAndCat.first)->SaveToKMLFile();
    pFW->Invalidate();
  }
}

EColor BookMarkManager::GetCurBookMarkColor() const
{
  Bookmark const * pBM = GetCurBookMark();
  if (pBM)
    return fromstringToColor(pBM->GetData().GetType());
  return CLR_RED;
}

bool BookMarkManager::IsCategoryVisible(int index)
{
  Framework * pFW = GetFramework();
  if (index >= pFW->GetBmCategoriesCount())
    return false;
  return pFW->GetBmCategory(index)->IsVisible();
}

void BookMarkManager::SetCategoryVisible(int index, bool bVisible)
{
  Framework * pFW = GetFramework();
  if (index >= pFW->GetBmCategoriesCount())
    return;
  pFW->GetBmCategory(index)->SetVisible(bVisible);
  pFW->GetBmCategory(index)->SaveToKMLFile();
  pFW->Invalidate();
}

void BookMarkManager::DeleteCategory(int index)
{
  Framework * pFW = GetFramework();
  if (index >= pFW->GetBmCategoriesCount())
    return;
  pFW->DeleteBmCategory(index);
  pFW->Invalidate();
}
size_t BookMarkManager::GetCategorySize(int index)
{
  Framework * pFW = GetFramework();
  if (index >= pFW->GetBmCategoriesCount())
    return 0;
  return pFW->GetBmCategory(index)->GetBookmarksCount();
}

namespace detail
{
Tizen::Base::String FormatSMSString(Tizen::Base::String message, Tizen::Base::String const & url)
{
  message.Replace("%1$s", url);
  String s2 = "http://ge0.me/";
  String s3;
  url.SubString(6, s3);
  s2.Append(s3);
  message.Replace("%2$s", s2);
  return message;
}

Tizen::Base::String FormatEmailString(Tizen::Base::String message, Tizen::Base::String const & description, Tizen::Base::String const & url)
{
  message.Replace("%1$s", description);
  message.Replace("%2$s", url);
  String s2 = "http://ge0.me/";
  String s3;
  url.SubString(6, s3);
  s2.Append(s3);
  message.Replace("%3$s", s2);
  return message;
}

}

Tizen::Base::String BookMarkManager::GetSMSTextMyPosition(double lat, double lon)
{
  Framework * pFW = GetFramework();
  String s = pFW->CodeGe0url(lat, lon, pFW->GetDrawScale(), "").c_str();
  String r = GetString(IDS_MY_POSITION_SHARE_SMS);
  return detail::FormatSMSString(r, s);
}

Tizen::Base::String BookMarkManager::GetSMSTextMark(UserMark const * pMark)
{
  if (!pMark)
    return "";
  double lat,lon;
  pMark->GetLatLon(lat, lon);
  Framework * pFW = GetFramework();
  String const s = pFW->CodeGe0url(lat, lon, pFW->GetDrawScale(), "").c_str();
  String const r = GetString(IDS_BOOKMARK_SHARE_SMS);
  return detail::FormatSMSString(r, s);
}

Tizen::Base::String BookMarkManager::GetEmailTextMyPosition(double lat, double lon)
{
  search::AddressInfo info;
  double y = MercatorBounds::LatToY(lat);
  double x = MercatorBounds::LonToX(lon);
  Framework * pFW = GetFramework();
  pFW->GetAddressInfoForGlobalPoint(m2::PointD(x, y), info);
  String const & description = info.FormatNameAndAddress().c_str();
  String const s = pFW->CodeGe0url(lat, lon, pFW->GetDrawScale(), "").c_str();
  String const r = GetString(IDS_MY_POSITION_SHARE_EMAIL);
  return detail::FormatEmailString(r, description, s);
}

Tizen::Base::String BookMarkManager::GetEmailTextMark(UserMark const * pMark)
{
  Framework * pFW = GetFramework();
  String const & description = GetMarkName(pMark);
  double lat,lon;
  pMark->GetLatLon(lat, lon);
  String const s = pFW->CodeGe0url(lat, lon, pFW->GetDrawScale(), "").c_str();
  String const r = GetString(IDS_MY_POSITION_SHARE_EMAIL);
  return detail::FormatEmailString(r, description, s);
}

}//bookmark
