#include "search/search_quality/assessment_tool/result_view.hpp"

#include "search/result.hpp"
#include "search/search_quality/assessment_tool/helpers.hpp"

#include <string>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>

using Relevance = search::Sample::Result::Relevance;

namespace
{
QLabel * CreateLabel(QWidget & parent)
{
  QLabel * label = new QLabel(&parent);
  label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  label->setWordWrap(true);
  return label;
}

void SetText(QLabel & label, std::string const & text) {
  if (text.empty())
  {
    label.hide();
    return;
  }

  label.setText(ToQString(text));
  label.show();
}
}  // namespace

ResultView::ResultView(search::Result const & result, QWidget & parent) : QWidget(&parent)
{
  Init();
  SetContents(result);
}

void ResultView::SetRelevance(Relevance relevance)
{
  m_irrelevant->setChecked(false);
  m_relevant->setChecked(false);
  m_vital->setChecked(false);
  switch (relevance)
  {
  case Relevance::Irrelevant: m_irrelevant->setChecked(true); break;
  case Relevance::Relevant: m_relevant->setChecked(true); break;
  case Relevance::Vital: m_vital->setChecked(true); break;
  }
}

void ResultView::Init()
{
  auto * layout = new QVBoxLayout(this /* parent */);
  layout->setSizeConstraint(QLayout::SetMinimumSize);
  setLayout(layout);

  m_string = CreateLabel(*this /* parent */);
  layout->addWidget(m_string);

  m_type = CreateLabel(*this /* parent */);
  m_type->setStyleSheet("QLabel { font-size : 8pt }");
  layout->addWidget(m_type);

  m_address = CreateLabel(*this /* parent */);
  m_address->setStyleSheet("QLabel { color : green ; font-size : 8pt }");
  layout->addWidget(m_address);

  {
    auto * group = new QWidget(this /* parent */);
    auto * groupLayout = new QHBoxLayout(group /* parent */);
    group->setLayout(groupLayout);

    m_irrelevant = new QRadioButton("0", this /* parent */);
    m_relevant = new QRadioButton("+1", this /* parent */);
    m_vital = new QRadioButton("+2", this /* parent */);

    groupLayout->addWidget(m_irrelevant);
    groupLayout->addWidget(m_relevant);
    groupLayout->addWidget(m_vital);

    layout->addWidget(group);
  }
}

void ResultView::SetContents(search::Result const & result)
{
  SetText(*m_string, result.GetString());
  SetText(*m_type, result.GetFeatureType());
  SetText(*m_address, result.GetAddress());

  m_irrelevant->setChecked(false);
  m_relevant->setChecked(false);
  m_vital->setChecked(false);
}
