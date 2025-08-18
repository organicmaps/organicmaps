#pragma once

#include "geometry/screenbase.hpp"

namespace df
{

bool CheckMinScale(ScreenBase const & screen);
bool CheckMaxScale(ScreenBase const & screen);
bool CheckMaxScale(ScreenBase const & screen, uint32_t tileSize, double visualScale);
bool CheckBorders(ScreenBase const & screen);

bool CanShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect);

ScreenBase const ShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect);
ScreenBase const ScaleInto(ScreenBase const & screen, m2::RectD const & boundRect);
ScreenBase const ShrinkAndScaleInto(ScreenBase const & screen, m2::RectD const & boundRect);

bool IsScaleAllowableIn3d(int scale);

double CalculateScale(m2::RectD const & pixelRect, m2::RectD const & localRect);

m2::PointD CalculateCenter(ScreenBase const & screen, m2::PointD const & userPos, m2::PointD const & pixelPos,
                           double azimuth);
m2::PointD CalculateCenter(double scale, m2::RectD const & pixelRect, m2::PointD const & userPos,
                           m2::PointD const & pixelPos, double azimuth);

bool ApplyScale(m2::PointD const & pixelScaleCenter, double factor, ScreenBase & screen);

}  // namespace df
