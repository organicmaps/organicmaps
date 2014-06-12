#pragma once

#include <FUi.h>
namespace bookmark
{
class BookMarkManager;
}

class SelectBMCategoryForm: public Tizen::Ui::Controls::Form
, public Tizen::Ui::Controls::IFormBackEventListener
, public Tizen::Ui::Controls::IListViewItemProviderF
, public Tizen::Ui::Controls::IListViewItemEventListener
, public Tizen::Ui::ITextEventListener
{
public:
  SelectBMCategoryForm();
  virtual ~SelectBMCategoryForm(void);

  bool Initialize(void);
  virtual result OnInitializing(void);
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form & source);
  //IListViewItemProvider
  virtual Tizen::Ui::Controls::ListItemBase * CreateItem (int index, float itemWidth);
  virtual bool  DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth);
  virtual int GetItemCount(void);
  // IListViewItemEventListener
  virtual void OnListViewContextItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListContextItemStatus state){}
  virtual void OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status);
  virtual void OnListViewItemSwept(Tizen::Ui::Controls::ListView & listView, int index, Tizen::Ui::Controls::SweepDirection direction) {}
  virtual void OnListViewItemLongPressed(Tizen::Ui::Controls::ListView & listView, int index, int elementId, bool & invokeListViewItemCallback) {}
  // ITextEventListener
  virtual void  OnTextValueChangeCanceled (Tizen::Ui::Control const & source);
  virtual void  OnTextValueChanged (Tizen::Ui::Control const & source);
};
