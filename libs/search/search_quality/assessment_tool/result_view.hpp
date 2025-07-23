#pragma once

#include "search/search_quality/assessment_tool/edits.hpp"
#include "search/search_quality/sample.hpp"

#include <memory>
#include <string>

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
  ResultView(search::Sample::Result const & result, QWidget & parent);

  void SetEditor(ResultsEdits::Editor && editor);

  void Update();

private:
  ResultView(std::string const & name, std::string const & type, std::string const & address, QWidget & parent);

  void Init();
  void SetContents(std::string const & name, std::string const & type, std::string const & address);

  QRadioButton * CreateRatioButton(std::string const & label, QLayout & layout);
  void OnRelevanceChanged();
  void UpdateRelevanceRadioButtons();

  QLabel * m_name = nullptr;
  QLabel * m_type = nullptr;
  QLabel * m_address = nullptr;

  QRadioButton * m_harmful = nullptr;
  QRadioButton * m_irrelevant = nullptr;
  QRadioButton * m_relevant = nullptr;
  QRadioButton * m_vital = nullptr;

  std::unique_ptr<ResultsEdits::Editor> m_editor;
};
