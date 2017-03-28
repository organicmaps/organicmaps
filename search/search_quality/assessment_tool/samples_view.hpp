#pragma once

#include "search/search_quality/sample.hpp"

#include <cstddef>
#include <vector>

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QTableView>

class SamplesView : public QTableView
{
public:
  explicit SamplesView(QWidget * parent);

  void SetSamples(std::vector<search::Sample> const & samples) { m_model->SetSamples(samples); }
  void OnSampleChanged(size_t index, bool hasEdits) { m_model->OnSampleChanged(index, hasEdits); }
  bool IsSelected(size_t index) const;

private:
  class Model : public QStandardItemModel
  {
  public:
    explicit Model(QWidget * parent);

    void SetSamples(std::vector<search::Sample> const & samples);
    void OnSampleChanged(size_t index, bool hasEdits);

    // QStandardItemModel overrides:
    QVariant data(QModelIndex const & index, int role = Qt::DisplayRole) const override;

  private:
    std::vector<bool> m_changed;
  };

  Model * m_model = nullptr;
};
