#pragma once

#include "search/search_quality/sample.hpp"

#include <vector>

#include <QtWidgets/QTableView>

class QStandardItemModel;

class SamplesView : public QTableView
{
public:
  explicit SamplesView(QWidget * parent);

  void SetSamples(std::vector<search::Sample> const & samples);

private:
  QStandardItemModel * m_model = nullptr;
};
