#pragma once

#include "search/search_quality/assessment_tool/edits.hpp"
#include "search/search_quality/sample.hpp"

#include <memory>

#include <QtWidgets/QWidget>

class QLabel;
class QRadioButton;

namespace search
{
class Result;
}

class ResultView : public QWidget
{
public:
  using Relevance = search::Sample::Result::Relevance;

  ResultView(search::Result const & result, QWidget & parent);

  void EnableEditing(Edits::RelevanceEditor && editor);

  void Update();

private:
  void Init();
  void SetContents(search::Result const & result);

  QRadioButton * CreateRatioButton(string const & label, QLayout & layout);
  void OnRelevanceChanged();

  QLabel * m_string = nullptr;
  QLabel * m_type = nullptr;
  QLabel * m_address = nullptr;

  QRadioButton * m_irrelevant = nullptr;
  QRadioButton * m_relevant = nullptr;
  QRadioButton * m_vital = nullptr;

  std::unique_ptr<Edits::RelevanceEditor> m_editor;
};
