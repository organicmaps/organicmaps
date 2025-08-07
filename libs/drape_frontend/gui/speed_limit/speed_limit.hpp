#pragma once

#include <string>

#include "drape/color.hpp"
#include "geometry/point2d.hpp"

namespace gui::speed_limit
{
class SpeedLimit
{
public:
  enum class Style : std::uint8_t
  {
    Vienna,
    Mutcd,

    Count
  };

  struct Config
  {
    float edgeWidthRatio;
    float outlineWidthRatio;

    dp::Color backgroundColor;
    dp::Color backgroundOutlineColor;
    dp::Color backgroundEdgeColor;
    dp::Color textColor;

    dp::Color backgroundColorAlert;
    dp::Color backgroundOutlineColorAlert;
    dp::Color backgroundEdgeColorAlert;
    dp::Color textColorAlert;
  };

  static constexpr Config kDefaultViennaStyleConfig = {
      .edgeWidthRatio = 0.02f,
      .outlineWidthRatio = 0.2f,

      .backgroundColor = dp::Color::White(),
      .backgroundOutlineColor = dp::Color::Red(),
      .backgroundEdgeColor = dp::Color::White(),
      .textColor = dp::Color::Black(),

      .backgroundColorAlert = dp::Color::Red(),
      .backgroundOutlineColorAlert = dp::Color::Red(),
      .backgroundEdgeColorAlert = dp::Color::Red(),
      .textColorAlert = dp::Color::White(),
  };

  static constexpr Config kDefaultMutcdStyleConfig = {
      .edgeWidthRatio = 0.02f,
      .outlineWidthRatio = 0.15f,

      .backgroundColor = dp::Color::White(),
      .backgroundOutlineColor = dp::Color::Black(),
      .backgroundEdgeColor = dp::Color::White(),
      .textColor = dp::Color::Black(),

      .backgroundColorAlert = dp::Color::Red(),
      .backgroundOutlineColorAlert = dp::Color::Red(),
      .backgroundEdgeColorAlert = dp::Color::Red(),
      .textColorAlert = dp::Color::White(),
  };

  void SetEnabled(bool enabled) { m_enabled = enabled; }
  void SetSpeedLimit(double speedLimitMps);
  void SetPosition(m2::PointF const & position) { m_position = position; }
  void SetSize(float size) { m_size = size; }

  bool IsEnabled() const { return m_enabled; }
  bool IsSpeedLimitAvailable() const;
  std::string GetSpeedLimit() const;
  m2::PointF const & GetPosition() const { return m_position; }
  float GetSize() const { return m_size; }

  Config const & GetConfig() const { return m_config[static_cast<std::size_t>(m_style)]; }

private:
  bool m_enabled = true;
  m2::PointF m_position = {1100.0, 1100.0};
  float m_size = 800.0f;
  Style m_style = Style::Vienna;
  std::array<Config, static_cast<std::size_t>(Style::Count)> m_config = {kDefaultViennaStyleConfig,
                                                                         kDefaultMutcdStyleConfig};

  double m_speedLimitMps = 120;
};
}  // namespace gui::speed_limit
