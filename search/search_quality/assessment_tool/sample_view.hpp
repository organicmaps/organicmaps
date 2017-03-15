#pragma once

#include "search/result.hpp"

#include <QtWidgets/QWidget>

class LanguagesList;
class QLineEdit;
class QListWidget;

namespace search
{
struct Sample;
}

class SampleView : public QWidget
{
public:
  explicit SampleView(QWidget * parent);

  void SetContents(search::Sample const & sample);
  void ShowResults(search::Results::Iter begin, search::Results::Iter end);

private:
  QLineEdit * m_query = nullptr;
  LanguagesList * m_langs = nullptr;
  QListWidget * m_results = nullptr;
};
