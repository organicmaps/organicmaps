#include "search/search_quality/helpers_json.hpp"

namespace m2
{
using std::string;

namespace
{
void ParsePoint(json_t * root, m2::PointD & point)
{
  FromJSONObject(root, "x", point.x);
  FromJSONObject(root, "y", point.y);
}
}  // namespace

void FromJSONObject(json_t * root, char const * field, RectD & rect)
{
  json_t * r = base::GetJSONObligatoryField(root, field);
  double minX, minY, maxX, maxY;
  FromJSONObject(r, "minx", minX);
  FromJSONObject(r, "miny", minY);
  FromJSONObject(r, "maxx", maxX);
  FromJSONObject(r, "maxy", maxY);
  rect.setMinX(minX);
  rect.setMinY(minY);
  rect.setMaxX(maxX);
  rect.setMaxY(maxY);
}

void ToJSONObject(json_t & root, char const * field, RectD const & rect)
{
  auto json = base::NewJSONObject();
  ToJSONObject(*json, "minx", rect.minX());
  ToJSONObject(*json, "miny", rect.minY());
  ToJSONObject(*json, "maxx", rect.maxX());
  ToJSONObject(*json, "maxy", rect.maxY());
  json_object_set_new(&root, field, json.release());
}

void FromJSONObject(json_t * root, string const & field, RectD & rect)
{
  FromJSONObject(root, field.c_str(), rect);
}

void ToJSONObject(json_t & root, string const & field, RectD const & rect)
{
  ToJSONObject(root, field.c_str(), rect);
}

void FromJSONObject(json_t * root, char const * field, PointD & point)
{
  json_t * p = base::GetJSONObligatoryField(root, field);
  ParsePoint(p, point);
}

void FromJSONObject(json_t * root, string const & field, PointD & point)
{
  FromJSONObject(root, field.c_str(), point);
}

void FromJSONObjectOptional(json_t * root, char const * field, std::optional<PointD> & point)
{
  json_t * p = base::GetJSONOptionalField(root, field);
  if (!p || base::JSONIsNull(p))
  {
    point = std::nullopt;
    return;
  }

  PointD parsed;
  ParsePoint(p, parsed);
  point = parsed;
}

void ToJSONObject(json_t & root, char const * field, PointD const & point)
{
  auto json = base::NewJSONObject();
  ToJSONObject(*json, "x", point.x);
  ToJSONObject(*json, "y", point.y);
  json_object_set_new(&root, field, json.release());
}

void FromJSONObjectOptional(json_t * root, string const & field, std::optional<PointD> & point)
{
  FromJSONObjectOptional(root, field.c_str(), point);
}

void ToJSONObject(json_t & root, string const & field, PointD const & point)
{
  ToJSONObject(root, field.c_str(), point);
}

void ToJSONObject(json_t & root, char const * field, std::optional<PointD> const & point)
{
  if (point)
    ToJSONObject(root, field, *point);
  else
    ToJSONObject(root, field, base::NewJSONNull());
}

void ToJSONObject(json_t & root, string const & field, std::optional<PointD> const & point)
{
  ToJSONObject(root, field.c_str(), point);
}
}  // namespace m2
