#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <optional>
#include <string>

#include "cppjansson/cppjansson.hpp"

namespace m2
{
void FromJSONObject(json_t * root, char const * field, RectD & rect);
void ToJSONObject(json_t & root, char const * field, RectD const & rect);
void FromJSONObject(json_t * root, std::string const & field, RectD & rect);
void ToJSONObject(json_t & root, std::string const & field, RectD const & rect);

void FromJSONObject(json_t * root, char const * field, PointD & point);
void FromJSONObjectOptional(json_t * root, char const * field, std::optional<PointD> & point);
void FromJSONObject(json_t * root, std::string const & field, PointD & point);
void FromJSONObjectOptional(json_t * root, std::string const & field, std::optional<PointD> & point);

void ToJSONObject(json_t & root, char const * field, PointD const & point);
void ToJSONObject(json_t & root, std::string const & field, PointD const & point);
void ToJSONObject(json_t & root, char const * field, std::optional<PointD> const & point);
void ToJSONObject(json_t & root, std::string const & field, std::optional<PointD> const & point);
}  // namespace m2
