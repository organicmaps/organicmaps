#pragma once

#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "3party/jansson/myjansson.hpp"

namespace generator
{
class JsonValue
{
public:
  explicit JsonValue(json_t * value = nullptr) : m_handle{value} { }
  explicit JsonValue(base::JSONPtr && value) : m_handle{std::move(value)} { }

  JsonValue(JsonValue const &) = delete;
  JsonValue & operator=(JsonValue const &) = delete;

  operator json_t const * () const noexcept { return m_handle.get(); }

  base::JSONPtr MakeDeepCopyJson() const
  {
    return base::JSONPtr{json_deep_copy(m_handle.get())};
  }

private:
  base::JSONPtr m_handle;
};

using KeyValue = std::pair<uint64_t, std::shared_ptr<JsonValue>>;

class KeyValueStorage
{
public:
  using JsonString = std::unique_ptr<char, JSONFreeDeleter>;

  explicit KeyValueStorage(std::string const & kvPath, size_t cacheValuesCountLimit,
                           std::function<bool(KeyValue const &)> const & pred = DefaultPred);

  KeyValueStorage(KeyValueStorage &&) = default;
  KeyValueStorage & operator=(KeyValueStorage &&) = default;

  KeyValueStorage(KeyValueStorage const &) = delete;
  KeyValueStorage & operator=(KeyValueStorage const &) = delete;

  void Insert(uint64_t key, JsonString && valueJson, base::JSONPtr && value = {});
  std::shared_ptr<JsonValue> Find(uint64_t key) const;
  size_t Size() const;

private:
  using Value = boost::variant<std::shared_ptr<JsonValue>, JsonString>;

  static bool DefaultPred(KeyValue const &) { return true; }
  static bool ParseKeyValueLine(std::string const & line, uint64_t & key, std::string & value,
                                std::streamoff lineNumber);
  JsonString CopyJsonString(std::string const & value) const;

  std::fstream m_storage;
  std::unordered_map<uint64_t, Value> m_values;
  size_t m_cacheValuesCountLimit;
};
}  // namespace generator
