#pragma once

#include <FUi.h>
#include "../../../std/map.hpp"
#include "../../../map/framework.hpp"

namespace storage
{
class Storage;
}

namespace Tizen{namespace Graphics
{
class Bitmap;
}}

class DownloadCountryForm: public Tizen::Ui::Controls::Form
, public Tizen::Ui::Controls::IFormBackEventListener
, public Tizen::Ui::IActionEventListener
, public Tizen::Ui::Scenes::ISceneEventListener
, public Tizen::Ui::Controls::IListViewItemProviderF
, public Tizen::Ui::Controls::IListViewItemEventListener
{
public:
  DownloadCountryForm();
  virtual ~DownloadCountryForm(void);

  bool Initialize(void);
  virtual result OnInitializing(void);
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form & source);
  virtual void OnActionPerformed(const Tizen::Ui::Control & source, int actionId);
  // ISceneEventListener
  virtual void OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId & previousSceneId,
      const Tizen::Ui::Scenes::SceneId & currentSceneId, Tizen::Base::Collection::IList * pArgs);
  virtual void OnSceneDeactivated(const Tizen::Ui::Scenes::SceneId & currentSceneId,
      const Tizen::Ui::Scenes::SceneId & nextSceneId);
  //IListViewItemProvider
  virtual Tizen::Ui::Controls::ListItemBase * CreateItem (int index, float itemWidth);
  virtual bool  DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth);
  virtual int GetItemCount(void);
  // IListViewItemEventListener
  virtual void OnListViewContextItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListContextItemStatus state);
  virtual void OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status);
  virtual void OnListViewItemSwept(Tizen::Ui::Controls::ListView & listView, int index, Tizen::Ui::Controls::SweepDirection direction);
  virtual void OnListViewItemLongPressed(Tizen::Ui::Controls::ListView & listView, int index, int elementId, bool & invokeListViewItemCallback);

private:
  Tizen::Graphics::Bitmap const * GetFlag(storage::TIndex const & country);
  bool IsGroup(storage::TIndex const & index) const;
  storage::TIndex GetIndex(int const ind) const;
  wchar_t const * GetNextScene() const;
  storage::Storage & Storage() const;
  void UpdateList();

  void OnCountryDownloaded(storage::TIndex const & country);
  void OnCountryDowloadProgres(storage::TIndex const & index, pair<int64_t, int64_t> const & p);

  enum EEventIDs
  {
    ID_FORMAT_STRING = 500,
    ID_FORMAT_FLAG,
    ID_FORMAT_STATUS,
    ID_FORMAT_DOWNLOADING_PROGR
  };

  map<storage::TIndex, Tizen::Graphics::Bitmap const *> m_flags;
  Tizen::Graphics::Bitmap const * m_downloadedBitmap;
  Tizen::Graphics::Bitmap const * m_updateBitmap;

  storage::TIndex m_groupIndex;
  Tizen::Base::String m_fromId;
  int m_dowloadStatusSlot;
  map<storage::TIndex, pair<int64_t, int64_t> > m_lastDownloadValue;
};
