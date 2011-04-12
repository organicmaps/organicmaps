#pragma once

#include "../geometry/cellid.hpp"
#include "../geometry/rect2d.hpp"

#include "../std/utility.hpp"


typedef double CoordT;
typedef pair<CoordT, CoordT> CoordPointT;

typedef m2::CellId<17> RectId;

m2::PointU PointD2PointU(CoordT x, CoordT y);
CoordPointT PointU2PointD(m2::PointU const & p);

int64_t PointToInt64(CoordT x, CoordT y);
inline int64_t PointToInt64(CoordPointT const & pt) { return PointToInt64(pt.first, pt.second); }
CoordPointT Int64ToPoint(int64_t v);

pair<int64_t, int64_t> RectToInt64(m2::RectD const & r);
m2::RectD Int64ToRect(pair<int64_t, int64_t> const & p);
