#pragma once

#include "kml/types.hpp"

#include "drape_frontend/user_marks_provider.hpp"

class Track : public df::UserLineMark
{
  using Base = df::UserLineMark;
public:
  explicit Track(kml::TrackData && data);

  bool IsDirty() const override { return m_isDirty; }
  void ResetChanges() const override { m_isDirty = false; }

  kml::TrackData const & GetData() const { return m_data; }

  std::string GetName() const;
  m2::RectD GetLimitRect() const;
  double GetLengthMeters() const;

  int GetMinZoom() const override { return 1; }
  df::DepthLayer GetDepthLayer() const override;
  size_t GetLayerCount() const override;
  dp::Color GetColor(size_t layerIndex) const override;
  float GetWidth(size_t layerIndex) const override;
  float GetDepth(size_t layerIndex) const override;
  std::vector<m2::PointD> const & GetPoints() const override;

  kml::MarkGroupId GetGroupId() const { return m_groupID; }

  void Attach(kml::MarkGroupId groupId);
  void Detach();

private:
  kml::TrackData m_data;
  kml::MarkGroupId m_groupID;
  mutable bool m_isDirty = true;
};
