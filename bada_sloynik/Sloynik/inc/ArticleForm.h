#ifndef ARTICLEFORM_H_
#define ARTICLEFORM_H_

#include <FBase.h>
#include <FUi.h>
#include <FWeb.h>
#include "../../../base/base.hpp"

class SearchForm;

class ArticleForm :
  public Osp::Ui::Controls::Form,
  public Osp::Ui::IActionEventListener
{
public:
  ArticleForm();
  virtual ~ArticleForm();
  bool Initialize(void);

  void MyInit();

  SearchForm * m_pSearchForm;
  Osp::Base::String m_Name;
  Osp::Base::ByteBuffer * m_pBuffer;

// Implementation
protected:
  enum
  {
    ID_SEARCH_SOFTKEY = 100
  };

  Osp::Web::Controls::Web * m_pWeb;

public:
  virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);
};

#endif /* ARTICLEFORM_H_ */
