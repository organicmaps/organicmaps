#pragma once

#include <FUi.h>
#include "../../../search/result.hpp"

class Framework;

class SearchForm: public Tizen::Ui::Controls::Form
, public Tizen::Ui::Controls::IFormBackEventListener
, public Tizen::Ui::IActionEventListener
, public Tizen::Ui::Controls::IScrollEventListener
, public Tizen::Ui::Controls::IListViewItemProviderF
, public Tizen::Ui::Controls::IListViewItemEventListener
, public Tizen::Ui::ITextEventListener
, public Tizen::Ui::IKeypadEventListener
, public Tizen::Ui::Scenes::ISceneEventListener
{
public:
  SearchForm();
  virtual ~SearchForm(void);

  bool Initialize(void);
private:

  virtual result OnInitializing(void);

  // ITextEventListener
  virtual void OnTextValueChanged(Tizen::Ui::Control const & source);
  virtual void OnTextValueChangeCanceled(Tizen::Ui::Control const & source){}
  // IFormBackEventListener
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form & source);
  // IActionEventListener
  virtual void OnActionPerformed(Tizen::Ui::Control const & source, int actionId);
  //IListViewItemProvider
  virtual Tizen::Ui::Controls::ListItemBase * CreateItem (int index, float itemWidth);
  virtual bool  DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth);
  virtual int GetItemCount(void);
  // IListViewItemEventListener
  virtual void OnListViewContextItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListContextItemStatus state){}
  virtual void OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status);
  virtual void OnListViewItemSwept(Tizen::Ui::Controls::ListView & listView, int index, Tizen::Ui::Controls::SweepDirection direction) {}
  virtual void OnListViewItemLongPressed(Tizen::Ui::Controls::ListView & listView, int index, int elementId, bool & invokeListViewItemCallback) {}
  // IScrollEventListener
  virtual void  OnScrollEndReached (Tizen::Ui::Control & source, Tizen::Ui::Controls::ScrollEndEvent type){};
  virtual void  OnScrollPositionChanged (Tizen::Ui::Control & source, int scrollPosition);
  // IKeypadEventListener
  virtual void  OnKeypadActionPerformed (Tizen::Ui::Control & source, Tizen::Ui::KeypadAction keypadAction);
  virtual void  OnKeypadBoundsChanged (Tizen::Ui::Control & source) {};
  virtual void  OnKeypadClosed (Tizen::Ui::Control & source){}
  virtual void  OnKeypadOpened (Tizen::Ui::Control & source){}
  virtual void  OnKeypadWillOpen (Tizen::Ui::Control & source){}
  // ISceneEventListener
  virtual void OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId& previousSceneId,
      const Tizen::Ui::Scenes::SceneId& currentSceneId, Tizen::Base::Collection::IList* pArgs);
  virtual void OnSceneDeactivated(const Tizen::Ui::Scenes::SceneId& currentSceneId,
      const Tizen::Ui::Scenes::SceneId& nextSceneId){}
  // search
  void OnSearchResultsReceived(search::Results const & results);

  void UpdateList();
  Tizen::Base::String GetSearchString() const;
  bool IsShowCategories() const;
  void Search(Tizen::Base::String const & val);

private:
  search::Results m_curResults;
  Tizen::Ui::Controls::SearchBar * m_searchBar;
};
