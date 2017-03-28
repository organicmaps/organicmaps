#pragma once

#include "search/search_quality/assessment_tool/context.hpp"

#include <cstddef>
#include <vector>

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QTableView>

class SamplesView : public QTableView
{
public:
  explicit SamplesView(QWidget * parent);

  void SetSamples(ContextList::SamplesSlice const & samples) { m_model->SetSamples(samples); }
  bool IsSelected(size_t index) const;

private:
  class Model : public QStandardItemModel
  {
  public:
    explicit Model(QWidget * parent);

    void SetSamples(ContextList::SamplesSlice const & samples)
    {
      removeRows(0, rowCount());
      m_samples = samples;
      if (m_samples.IsValid())
        insertRows(0, m_samples.Size());
    }

    // QStandardItemModel overrides:
    QVariant data(QModelIndex const & index, int role = Qt::DisplayRole) const override;

  private:
    ContextList::SamplesSlice m_samples;
  };

  Model * m_model = nullptr;
};
