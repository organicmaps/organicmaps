#pragma once

#include "geometry/rect2d.hpp"

#include <cstdint>
#include <string>

#include "3party/jansson/myjansson.hpp"

namespace search
{
// todo(@m) We should not need that much.
size_t constexpr kMaxOpenFiles = 4000;

void ChangeMaxNumberOfOpenFiles(size_t n);

void CheckLocale();
}  // namespace search

namespace m2
{
void FromJSONObject(json_t * root, char const * field, RectD & rect);
void ToJSONObject(json_t & root, char const * field, RectD const & rect);
void FromJSONObject(json_t * root, std::string const & field, RectD & rect);
void ToJSONObject(json_t & root, std::string const & field, RectD const & rect);

void FromJSONObject(json_t * root, char const * field, PointD & point);
bool FromJSONObjectOptional(json_t * root, char const * field, PointD & point);
void FromJSONObject(json_t * root, std::string const & field, PointD & point);
bool FromJSONObjectOptional(json_t * root, std::string const & field, PointD & point);

void ToJSONObject(json_t & root, char const * field, PointD const & point);
void ToJSONObject(json_t & root, std::string const & field, PointD const & point);
}  // namespace m2
