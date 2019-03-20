#include "search/search_quality/helpers.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include <memory>
#include <utility>

#include <sys/resource.h>

using namespace std;

namespace
{
void ParsePoint(json_t * root, m2::PointD & point)
{
  FromJSONObject(root, "x", point.x);
  FromJSONObject(root, "y", point.y);
}
}  // namespace

namespace search
{
void ChangeMaxNumberOfOpenFiles(size_t n)
{
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_LINUX)
  struct rlimit rlp;
  getrlimit(RLIMIT_NOFILE, &rlp);
  rlp.rlim_cur = n;
  setrlimit(RLIMIT_NOFILE, &rlp);
#endif
}

void CheckLocale()
{
  string const kJson = "{\"coord\":123.456}";
  string const kErrorMsg = "Bad locale. Consider setting LC_ALL=C";

  double coord;
  {
    base::Json root(kJson.c_str());
    FromJSONObject(root.get(), "coord", coord);
  }

  string line;
  {
    auto root = base::NewJSONObject();
    ToJSONObject(*root, "coord", coord);

    unique_ptr<char, JSONFreeDeleter> buffer(
        json_dumps(root.get(), JSON_COMPACT | JSON_ENSURE_ASCII));

    line.append(buffer.get());
  }

  CHECK_EQUAL(line, kJson, (kErrorMsg));

  {
    string const kTest = "123.456";
    double value;
    VERIFY(strings::to_double(kTest, value), (kTest));
    CHECK_EQUAL(strings::to_string(value), kTest, (kErrorMsg));
  }
}
}  // namespace search

namespace m2
{
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

void FromJSONObjectOptional(json_t * root, char const * field, boost::optional<PointD> & point)
{
  json_t * p = base::GetJSONOptionalField(root, field);
  if (!p || base::JSONIsNull(p))
  {
    point = boost::none;
    return;
  }

  PointD parsed;
  ParsePoint(p, parsed);
  point = move(parsed);
}

void ToJSONObject(json_t & root, char const * field, PointD const & point)
{
  auto json = base::NewJSONObject();
  ToJSONObject(*json, "x", point.x);
  ToJSONObject(*json, "y", point.y);
  json_object_set_new(&root, field, json.release());
}

void FromJSONObjectOptional(json_t * root, string const & field, boost::optional<PointD> & point)
{
  FromJSONObjectOptional(root, field.c_str(), point);
}

void ToJSONObject(json_t & root, string const & field, PointD const & point)
{
  ToJSONObject(root, field.c_str(), point);
}

void ToJSONObject(json_t & root, char const * field, boost::optional<PointD> const & point)
{
  if (point)
    ToJSONObject(root, field, *point);
  else
    ToJSONObject(root, field, base::NewJSONNull());
}

void ToJSONObject(json_t & root, std::string const & field, boost::optional<PointD> const & point)
{
  ToJSONObject(root, field.c_str(), point);
}
}  // namespace m2
