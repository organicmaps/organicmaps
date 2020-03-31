#pragma once

#include "kml/types.hpp"

#include "drape_frontend/user_marks_provider.hpp"

#include <string>

class Track : public df::UserLineMark
{
  using Base = df::UserLineMark;
public:
  Track(kml::TrackData && data, bool interactive);

  kml::MarkGroupId GetGroupId() const override { return m_groupID; }

  bool IsDirty() const override { return m_isDirty; }
  void ResetChanges() const override { m_isDirty = false; }

  kml::TrackData const & GetData() const { return m_data; }

  std::string GetName() const;
  m2::RectD GetLimitRect() const;
  double GetLengthMeters() const;
  double GetLengthMeters(size_t pointIndex) const;
  bool IsInteractive() const;

  int GetMinZoom() const override { return 1; }
  df::DepthLayer GetDepthLayer() const override;
  size_t GetLayerCount() const override;
  dp::Color GetColor(size_t layerIndex) const override;
  float GetWidth(size_t layerIndex) const override;
  float GetDepth(size_t layerIndex) const override;
  std::vector<m2::PointD> GetPoints() const override;
  std::vector<geometry::PointWithAltitude> const & GetPointsWithAltitudes() const;

  void Attach(kml::MarkGroupId groupId);
  void Detach();

  bool GetPoint(double distanceInMeters, m2::PointD & pt) const;

private:
  void CacheDataForInteraction();
  bool HasAltitudes() const;
  std::vector<double> GetLengthsImpl() const;
  m2::RectD GetLimitRectImpl() const;

  kml::TrackData m_data;
  kml::MarkGroupId m_groupID = kml::kInvalidMarkGroupId;

  struct InteractionData
  {
    std::vector<double> m_lengths;
    m2::RectD m_limitRect;
  };
  std::optional<InteractionData> m_interactionData;

  mutable bool m_isDirty = true;
};
