#ifndef _SEARCHFORM_H_
#define _SEARCHFORM_H_

#include <FBase.h>
#include <FUi.h>
#include "../../../base/base.hpp"

class ArticleForm;

namespace sl
{
class SloynikEngine;
}

class SearchForm :
	public Osp::Ui::Controls::Form,
	public Osp::Ui::IActionEventListener,
	public Osp::Ui::ITextEventListener,
	public Osp::Ui::ISlidableListEventListener,
	public Osp::Ui::ICustomItemEventListener
{

// Construction
public:
	SearchForm(void);
	virtual ~SearchForm(void);
	bool Initialize(void);

// Implementation
protected:
	sl::SloynikEngine * m_pEngine;
	ArticleForm * m_pArticleForm;

	Osp::Ui::Controls::EditField * m_pSearchField;
	Osp::Ui::Controls::SlidableList * m_pResultsList;
	Osp::Ui::Controls::CustomListItemFormat * m_pCustomListItemFormat;

	static const int ID_CLEAR_SEARCH_FIELD = 117;

  static const int TEXT_ID  = 101;
  static const int ITEM_HEIGHT = 40;

  Osp::Ui::Controls::CustomListItem * CreateListItem(uint32_t id);

public:
	virtual result OnInitializing(void);
	virtual result OnTerminating(void);
	virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

  virtual void OnTextValueChanged(Osp::Ui::Control const & source);
  virtual void OnTextValueChangeCanceled(Osp::Ui::Control const & source);

  virtual void OnListPropertyRequested(const Osp::Ui::Control&);
  virtual void OnLoadToTopRequested(const Osp::Ui::Control&, int, int);
  virtual void OnLoadToBottomRequested(const Osp::Ui::Control&, int, int);
  virtual void OnUnloadItemRequested(const Osp::Ui::Control&, int);

  virtual void OnItemStateChanged(const Osp::Ui::Control &source, int index,
      int itemId, int elementId, Osp::Ui::ItemStatus status);
  virtual void OnItemStateChanged(const Osp::Ui::Control &source, int index,
      int itemId, Osp::Ui::ItemStatus status);
};

#endif	//_SEARCHFORM_H_
