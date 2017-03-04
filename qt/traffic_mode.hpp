#pragma once

#include "indexer/feature.hpp"

#include "openlr/openlr_model.hpp"
#include "openlr/openlr_sample.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include <QtCore/QAbstractTableModel>

class Index;
class QItemSelection;

struct DecodedSample
{
  DecodedSample(Index const & index, openlr::SamplePool const & sample);

  openlr::SamplePool const & GetItems() const { return m_decodedItems; }
  std::vector<m2::PointD> GetPoints(size_t const index) const;

  std::map<FeatureID, std::vector<m2::PointD>> m_points;
  std::vector<openlr::SampleItem> m_decodedItems;
};

/// This class is used to delegate segments drawing to the DrapeEngine.
class TrafficDrawerDelegateBase
{
public:
  virtual ~TrafficDrawerDelegateBase() = default;

  virtual void SetViewportCenter(m2::PointD const & center) = 0;

  virtual void DrawDecodedSegments(DecodedSample const & sample, int sampleIndex) = 0;
  virtual void DrawEncodedSegment(openlr::LinearSegment const & segment) = 0;
  virtual void Clear() = 0;
};

/// This class is used to map sample ids to real data
/// and change sample evaluations.
class TrafficMode : public QAbstractTableModel
{
  Q_OBJECT

public:
  TrafficMode(std::string const & dataFileName, std::string const & sampleFileName,
              Index const & index, std::unique_ptr<TrafficDrawerDelegateBase> drawerDelagate,
              QObject * parent = Q_NULLPTR);

  bool SaveSampleAs(std::string const & fileName) const;
  bool IsValid() const { return m_valid; }

  int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex & index, int role) const Q_DECL_OVERRIDE;
  // QVariant headerData(int section, Qt::Orientation orientation,
  //                     int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  Qt::ItemFlags flags(QModelIndex const & index) const Q_DECL_OVERRIDE;
  bool setData(QModelIndex const & index, QVariant const & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

public slots:
  void OnItemSelected(QItemSelection const & selected, QItemSelection const &);

private:
  std::unique_ptr<DecodedSample> m_decodedSample;
  std::unordered_map<decltype(openlr::LinearSegment::m_segmentId), openlr::LinearSegment> m_partnerSegments;

  std::unique_ptr<TrafficDrawerDelegateBase> m_drawerDelegate;

  bool m_valid = false;
};
