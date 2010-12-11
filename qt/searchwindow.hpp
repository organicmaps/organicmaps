#pragma once

#include "../indexer/feature.hpp"

#include "../std/vector.hpp"

#include <QtGui/QLineEdit>
#include <QtGui/QTableWidget>

namespace model
{
  class FeatureVector;
  class FeaturesFetcher;
}

namespace qt
{
  // edit box class
  typedef QLineEdit FindEditorWnd;

  // feature names table class
  class FindTableWnd : public QTableWidget
  {
    typedef QTableWidget base_type;

    Q_OBJECT

    //typedef model::FeatureVector model_t;
    typedef model::FeaturesFetcher model_t;

    FindEditorWnd * m_pEditor;
    model_t * m_pModel;

    typedef FeatureGeom feature_t;

  public:
    FindTableWnd(QWidget * pParent, FindEditorWnd * pEditor, model_t * pModel);

    feature_t const & GetFeature(size_t row) const;

  protected:
    bool AddFeature(feature_t const & f);

  protected Q_SLOTS:
    void OnTextChanged(QString const & s);

  private:
    vector<feature_t> m_features;
  };
}
