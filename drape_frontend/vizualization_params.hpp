#pragma once

namespace df
{
  class VizualizationParams
  {
  public:
    static double GetVisualScale();
    static void SetVisualScale(double visualScale);

  private:
    static double m_visualScale;
  };
}
