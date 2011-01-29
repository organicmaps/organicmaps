#pragma once

#include "../geometry/point2d.hpp"
#include "../base/base.hpp"
#include "../std/vector.hpp"
#include "../std/tuple.hpp"

void EncodePolyline(vector<m2::PointU> const & points,
                    m2::PointU const & basePoint,
                    m2::PointU const & maxPoint,
                    vector<char> & serialOutput);

void DecodePolyline(char const * pBeg, char const * pEnd,
                    m2::PointU const & basePoint,
                    m2::PointU const & maxPoint,
                    vector<m2::PointU> & points);

void EncodeTriangles(vector<m2::PointU> const & points,
                     vector<tuple<uint32_t, uint32_t, uint32_t> > const & triangles,
                     m2::PointU const & basePoint,
                     m2::PointU const & maxPoint,
                     vector<char> & serialOutput);

void DecodeTriangles(char const * pBeg, char const * pEnd,
                     m2::PointU const & basePoint,
                     m2::PointU const & maxPoint,
                     vector<m2::PointU> & points,
                     vector<tuple<uint32_t, uint32_t, uint32_t> > & triangles);

void EncodeTriangleStrip(vector<m2::PointU> const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<char> & serialOutput);

void DecodeTriangleStrip(char const * pBeg, char const * pEnd,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<m2::PointU> & points);
