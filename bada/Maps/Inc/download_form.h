#pragma once

#include <FUi.h>

class MapsGl;

class DownloadForm : public Osp::Ui::Controls::Form,
    public Osp::Ui::IGroupedItemEventListener,
    public Osp::Ui::IActionEventListener
//    public Osp::Ui::IFastScrollEventListener
{
  static const int ID_BACK_SOFTKEY = 100;

  MapsGl & m_mapsGl;

public:
  static const RequestId REQUEST_MAINFORM = 100;

  DownloadForm(MapsGl & mapsGl);
  virtual ~DownloadForm(void);

  bool Initialize(void);

protected:

  static const int ID_FORMAT_STRING = 501;
  static const int ID_CUSTOMLIST_ITEM1 = 503;
  static const int ID_CUSTOMLIST_ITEM2 = 504;
  static const int ID_CUSTOMLIST_ITEM3 = 505;
  static const int ID_CUSTOMLIST_ITEM4 = 506;
  static const int ID_CUSTOMLIST_ITEM5 = 507;

  result ReFillList(Osp::Ui::Controls::GroupedList & list);
  result ShowCountryAndClose(int groupIndex, int itemIndex);

  //	Osp::Ui::Controls::Label* __pLabelLog;
  Osp::Ui::Controls::CustomListItemFormat* __pItemFormat;
  Osp::Ui::Controls::GroupedList* __pGroupedList;

  virtual result OnInitializing(void);
  virtual result OnTerminating(void);

  virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

  virtual void OnItemStateChanged(const Osp::Ui::Control &source,
      int groupIndex, int itemIndex, int itemId, int elementId,
      Osp::Ui::ItemStatus status);
  virtual void OnItemStateChanged(const Osp::Ui::Control &source,
      int groupIndex, int itemIndex, int itemId, Osp::Ui::ItemStatus status);
//  virtual void OnMainIndexChanged(const Osp::Ui::Control &source,
//      Osp::Base::Character &mainIndex);
//  virtual void OnSubIndexChanged(const Osp::Ui::Control &source,
//      Osp::Base::Character &mainIndex, Osp::Base::Character &subIndex);
//  virtual void OnMainIndexSelected(const Osp::Ui::Control &source,
//      Osp::Base::Character &mainIndex);
//  virtual void OnSubIndexSelected(const Osp::Ui::Control &source,
//      Osp::Base::Character &mainIndex, Osp::Base::Character &subIndex);
};
