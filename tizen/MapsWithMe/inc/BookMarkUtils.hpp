#pragma once

#include <FBase.h>
#include "../../../map/user_mark.hpp"
#include "../../../std/noncopyable.hpp"
#include "../../../std/shared_ptr.hpp"

class UserMark;
class Bookmark;
namespace bookmark
{

enum EColor
  {
    CLR_RED,
    CLR_BLUE,
    CLR_BROWN,
    CLR_GREEN,
    CLR_ORANGE,
    CLR_PINK,
    CLR_PURPLE,
    CLR_YELLOW
  };

search::AddressInfo const GetAdressInfo(UserMark const * pUserMark);
Tizen::Base::String GetMarkName(UserMark const * pUserMark);
Tizen::Base::String GetMarkType(UserMark const * pUserMark);
Tizen::Base::String GetMarkCountry(UserMark const * pUserMark);
Tizen::Base::String GetDistance(UserMark const * pUserMark);
double GetAzimuth(UserMark const * pUserMark, double north);

const wchar_t * GetColorBM(EColor color);
const wchar_t * GetColorPPBM(EColor color);
const wchar_t * GetColorSelecteBM(EColor color);
string fromEColorTostring(EColor color);
EColor fromstringToColor(string const & sColor);

bool IsBookMark(UserMark const * pUserMark);

class BookMarkManager: public noncopyable
{
private:
  BookMarkManager();
public:

  static BookMarkManager & GetInstance();

  //current bookmark
  UserMark const * GetCurMark() const;
  Bookmark const * GetCurBookMark() const;
  void RemoveCurBookMark();
  EColor GetCurBookMarkColor() const;
  void ActivateBookMark(UserMarkCopy * pCopy);
  void AddCurMarkToBookMarks();
  Tizen::Base::String GetBookMarkMessage() const;
  void SetBookMarkMessage(Tizen::Base::String const & s);
  Tizen::Base::String GetCurrentCategoryName() const;
  int GetCurrentCategory() const;
  void SetNewCurBookMarkCategory(int const nNewCategory);
  void SetCurBookMarkColor(EColor const color);

  // any bookmark
  void DeleteBookMark(size_t category, size_t index);
  void ShowBookMark(size_t category, size_t index);
  Bookmark const * GetBookMark(size_t category, size_t index);

  // category
  int AddCategory(Tizen::Base::String const & sName) const;
  bool IsCategoryVisible(int index);
  void SetCategoryVisible(int index, bool bVisible);
  void DeleteCategory(int index);
  size_t GetCategorySize(int index);
  int GetCategoriesCount() const;
  Tizen::Base::String GetCategoryName(int const index) const;
  void SetCategoryName(int const index, Tizen::Base::String const & sName) const;

  Tizen::Base::String GetSMSTextMyPosition(double lat, double lon);
  Tizen::Base::String GetSMSTextMark(UserMark const * pMark);
  Tizen::Base::String GetEmailTextMyPosition(double lat, double lon);
  Tizen::Base::String GetEmailTextMark(UserMark const * pMark);

private:
  shared_ptr<UserMarkCopy> m_pCurBookMarkCopy;
};

BookMarkManager & GetBMManager();
}
