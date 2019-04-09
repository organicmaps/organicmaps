#pragma once

#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/optional.hpp>

#include "3party/jansson/myjansson.hpp"

namespace generator
{
namespace geo_objects
{
using KeyValue = std::pair<uint64_t, base::Json>;

class KeyValueStorage
{
public:
  explicit KeyValueStorage(std::istream & stream,
                           std::function<bool(KeyValue const &)> const & pred = DefaultPred);

  boost::optional<base::Json> Find(uint64_t key) const;
  size_t Size() const;

private:
  static bool DefaultPred(KeyValue const &) { return true; }
  static bool ParseKeyValueLine(std::string const & line, KeyValue & res, std::streamoff lineNumber);

  std::unordered_map<uint64_t, base::Json> m_values;
};
}  // namespace geo_objects
}  // namespace generator
