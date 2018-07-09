#pragma once

#include "coding/pointd_to_pointu.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstdint>
#include <utility>

// All functions in this file are deprecated and are left
// only for backward compatibility.
//
// Their intention was to store a point with unsigned 32-bit integer
// coordinates to a signed or to an unsigned 64-bit integer by interleaving the
// bits of the point's coordinates.
//
// A possible reason for interleaving is to lower the number of bytes
// needed by the varint encoding, at least if the coordinates are of the
// same order of magnitude. However, this is hard to justify:
// 1. We have no reason to expect the coordinates to be of the same order.
// 2. If you need to serialize a point, doing it separately
//    for each coordinate is almost always a better option.
// 3. If you need to temporarily store the point as an uint,
//    you do not need the complexity of interleaving.
//
// Another possible reason to interleave bits of x and y arises
// when implementing the Z-order curve but we have this
// written elsewhere (see geometry/cellid.hpp).

int64_t PointToInt64Obsolete(double x, double y, uint32_t coordBits);

int64_t PointToInt64Obsolete(m2::PointD const & pt, uint32_t coordBits);

m2::PointD Int64ToPointObsolete(int64_t v, uint32_t coordBits);

std::pair<int64_t, int64_t> RectToInt64Obsolete(m2::RectD const & r, uint32_t coordBits);

m2::RectD Int64ToRectObsolete(std::pair<int64_t, int64_t> const & p, uint32_t coordBits);

uint64_t PointUToUint64Obsolete(m2::PointU const & pt);

m2::PointU Uint64ToPointUObsolete(int64_t v);
