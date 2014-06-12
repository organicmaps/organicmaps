#pragma once

#include <FUi.h>

class CategoryForm: public Tizen::Ui::Controls::Form
, public Tizen::Ui::Controls::IFormBackEventListener
, public Tizen::Ui::IActionEventListener
, public Tizen::Ui::Controls::IListViewItemProviderF
, public Tizen::Ui::Controls::IListViewItemEventListener
, public Tizen::Ui::Scenes::ISceneEventListener
, public Tizen::Ui::ITextEventListener
{
public:
  CategoryForm();
  virtual ~CategoryForm(void);

  bool Initialize(void);
  virtual result OnInitializing(void);
  //IListViewItemProvider
  virtual Tizen::Ui::Controls::ListItemBase * CreateItem (int index, float itemWidth);
  virtual bool  DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth);
  virtual int GetItemCount(void);
  // IListViewItemEventListener
  virtual void OnListViewContextItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListContextItemStatus state){}
  virtual void OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status);
  virtual void OnListViewItemSwept(Tizen::Ui::Controls::ListView & listView, int index, Tizen::Ui::Controls::SweepDirection direction) {}
  virtual void OnListViewItemLongPressed(Tizen::Ui::Controls::ListView & listView, int index, int elementId, bool & invokeListViewItemCallback) {}
  // ISceneEventListener
  virtual void OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId & previousSceneId,
      const Tizen::Ui::Scenes::SceneId & currentSceneId, Tizen::Base::Collection::IList * pArgs);
  virtual void OnSceneDeactivated(const Tizen::Ui::Scenes::SceneId & currentSceneId,
      const Tizen::Ui::Scenes::SceneId & nextSceneId){}
  // Tizen::Ui::ITextEventListener
  virtual void  OnTextValueChangeCanceled (Tizen::Ui::Control const & source){}
  virtual void  OnTextValueChanged (Tizen::Ui::Control const & source);

  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form & source);
  virtual void OnActionPerformed(Tizen::Ui::Control const & source, int actionId);

  void UpdateState();
  enum EElementID
  {
    ID_DELETE,
    ID_COLOR,
    ID_NAME,
    ID_DISTANCE,
    ID_DELETE_TXT
  };
  enum EActionID
  {
    ID_VISIBILITY_ON,
    ID_VISIBILITY_OFF,
    ID_EDIT
  };

  bool m_bEditState;
  int m_itemToDelete;
  int m_curCategory;
};
