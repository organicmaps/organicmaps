#pragma once

#include "geometry/screenbase.hpp"

namespace df
{

bool CheckMinScale(ScreenBase const & screen);
bool CheckMaxScale(ScreenBase const & screen);
bool CheckMaxScale(ScreenBase const & screen, uint32_t tileSize, double visualScale);
bool CheckBorders(ScreenBase const & screen);

bool CanShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect);

void ShrinkInto(ScreenBase & screen, m2::RectD const & boundRect);
void ScaleInto(ScreenBase & screen, m2::RectD const & boundRect);
void ShrinkAndScaleInto(ScreenBase & screen, m2::RectD const & boundRect);

bool IsScaleAllowableIn3d(int scale);

double CalculateScale(m2::RectD const & pixelRect, m2::RectD const & localRect);

m2::PointD CalculateCenter(ScreenBase const & screen, m2::PointD const & userPos, m2::PointD const & pixelPos,
                           double azimuth);
m2::PointD CalculateCenter(double scale, m2::RectD const & pixelRect, m2::PointD const & userPos,
                           m2::PointD const & pixelPos, double azimuth);

bool ApplyScale(m2::PointD const & pixelScaleCenter, double factor, ScreenBase & screen);

/// Wraps the screen origin X into [-540, 540] to prevent unbounded coordinate growth
/// when scrolling continuously past the antimeridian. The +-540 threshold (1.5 world widths)
/// gives a buffer before normalization kicks in, minimizing tile cache churn.
void NormalizeScreenOriginX(ScreenBase & screen);

/// Adjusts a point's X coordinate to be within 180 degrees of the screen origin,
/// for correct rendering when the viewport extends past the antimeridian.
m2::PointD AdjustPointForViewport(m2::PointD const & pt, ScreenBase const & screen);

}  // namespace df
