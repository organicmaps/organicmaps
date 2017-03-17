#include "search/search_quality/assessment_tool/sample_view.hpp"

#include "search/result.hpp"
#include "search/search_quality/assessment_tool/helpers.hpp"
#include "search/search_quality/assessment_tool/languages_list.hpp"
#include "search/search_quality/sample.hpp"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>

namespace
{
QWidget * CreateSampleWidget(QWidget & parent, search::Result const & result)
{
  auto * widget = new QWidget(&parent);
  auto * layout = new QVBoxLayout(widget);
  widget->setLayout(layout);

  layout->addWidget(new QLabel(ToQString(result.GetString())));

  if (result.GetResultType() == search::Result::RESULT_FEATURE && !result.GetFeatureType().empty())
  {
    auto * type = new QLabel(ToQString(result.GetFeatureType()), widget);
    type->setStyleSheet("QLabel { font-size : 8pt }");
    layout->addWidget(type);
  }

  if (!result.GetAddress().empty())
  {
    auto * address = new QLabel(ToQString(result.GetAddress()), widget);
    address->setStyleSheet("QLabel { color : green ; font-size : 8pt }");
    layout->addWidget(address);
  }

  return widget;
}
}  // namespace

SampleView::SampleView(QWidget * parent) : QWidget(parent)
{
  auto * mainLayout = BuildLayoutWithoutMargins<QVBoxLayout>(this /* parent */);

  {
    auto * box = new QWidget(this /* parent */);
    auto * layout = BuildLayoutWithoutMargins<QHBoxLayout>(box /* parent */);
    box->setLayout(layout);

    m_query = new QLineEdit(this /* parent */);
    m_query->setToolTip(tr("Query text"));
    layout->addWidget(m_query);

    m_langs = new LanguagesList(this /* parent */);
    m_langs->setToolTip(tr("Query input language"));
    layout->addWidget(m_langs);

    mainLayout->addWidget(box);
  }

  {
    auto * box = new QWidget(this /* parent */);
    auto * layout = BuildLayoutWithoutMargins<QVBoxLayout>(box /* parent */);
    box->setLayout(layout);

    layout->addWidget(new QLabel(tr("Found results")));

    m_results = new QListWidget(box /* parent */);
    layout->addWidget(m_results);

    mainLayout->addWidget(box);
  }

  setLayout(mainLayout);
  setWindowTitle(tr("Sample"));
}

void SampleView::SetContents(search::Sample const & sample)
{
  m_query->setText(ToQString(sample.m_query));
  m_query->home(false /* mark */);

  m_langs->Select(sample.m_locale);

  m_results->clear();
}

void SampleView::ShowResults(search::Results::Iter begin, search::Results::Iter end)
{
  for (auto it = begin; it != end; ++it)
  {
    auto * item = new QListWidgetItem(m_results);
    m_results->addItem(item);

    auto * widget = CreateSampleWidget(*m_results /* parent */, *it /* sample */);
    item->setSizeHint(widget->minimumSizeHint());

    m_results->setItemWidget(item, widget);
  }
}
