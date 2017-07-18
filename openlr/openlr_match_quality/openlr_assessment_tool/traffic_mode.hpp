#pragma once

#include "indexer/feature.hpp"

#include "openlr/decoded_path.hpp"
#include "openlr/openlr_model.hpp"

#include "base/exception.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/optional.hpp>

#include <QAbstractTableModel>

class Index;
class QItemSelection;
class Selection;

DECLARE_EXCEPTION(TrafficModeError, RootException);

class SegmentCorrespondence
{
public:
  boost::optional<openlr::Path> const & GetMatchedPath() const { return m_matchedPath; }
  boost::optional<openlr::Path> & GetMatchedPath() { return m_matchedPath; }

  boost::optional<openlr::Path> const & GetFakePath() const { return m_fakePath; }
  boost::optional<openlr::Path> & GetFakePath() { return m_fakePath; }

  openlr::LinearSegment const & GetPartnerSegment() const { return m_partnerSegment; }
  openlr::LinearSegment & GetPartnerSegment() { return m_partnerSegment; }

  uint32_t GetPartnerSegmentId() const { return m_partnerSegment.m_segmentId; }

private:
  openlr::LinearSegment m_partnerSegment;
  boost::optional<openlr::Path> m_matchedPath;
  boost::optional<openlr::Path> m_fakePath;
};

/// This class is used to delegate segments drawing to the DrapeEngine.
class TrafficDrawerDelegateBase
{
public:
  virtual ~TrafficDrawerDelegateBase() = default;

  virtual void SetViewportCenter(m2::PointD const & center) = 0;

  virtual void DrawDecodedSegments(std::vector<m2::PointD> const & points) = 0;
  virtual void DrawEncodedSegment(std::vector<m2::PointD> const & points) = 0;
  virtual void Clear() = 0;
};

/// This class is used to map sample ids to real data
/// and change sample evaluations.
class TrafficMode : public QAbstractTableModel
{
  Q_OBJECT

public:
  // TODO(mgsergio): Check we are on the right mwm. I.E. right mwm version an everything.
  TrafficMode(std::string const & dataFileName, Index const & index,
              std::unique_ptr<TrafficDrawerDelegateBase> drawerDelagate,
              QObject * parent = Q_NULLPTR);

  bool SaveSampleAs(std::string const & fileName) const;

  int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex & index, int role) const Q_DECL_OVERRIDE;
  // QVariant headerData(int section, Qt::Orientation orientation,
  //                     int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  Qt::ItemFlags flags(QModelIndex const & index) const Q_DECL_OVERRIDE;

public slots:
  void OnItemSelected(QItemSelection const & selected, QItemSelection const &);

private:
  Index const & m_index;
  std::vector<SegmentCorrespondence> m_segments;

  std::unique_ptr<TrafficDrawerDelegateBase> m_drawerDelegate;
};
