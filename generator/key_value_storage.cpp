#include "generator/key_value_storage.hpp"

#include "coding/reader.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include <cstring>

namespace generator
{
KeyValueStorage::KeyValueStorage(std::string const & path, size_t cacheValuesCountLimit,
                                 std::function<bool(KeyValue const &)> const & pred)
  : m_storage{path, std::ios_base::in | std::ios_base::out | std::ios_base::app}
  , m_cacheValuesCountLimit{cacheValuesCountLimit}
{
  if (!m_storage)
    MYTHROW(Reader::OpenException, ("Failed to open file", path));

  std::string line;
  std::streamoff lineNumber = 0;
  while (std::getline(m_storage, line))
  {
    ++lineNumber;

    uint64_t key;
    auto value = std::string{};
    if (!ParseKeyValueLine(line, lineNumber, key, value))
      continue;

    json_error_t jsonError;
    auto json = std::make_shared<JsonValue>(json_loads(value.c_str(), 0, &jsonError));
    if (!json)
    {
      LOG(LWARNING, ("Cannot create base::Json in line", lineNumber, ":", jsonError.text));
      continue;
    }

    if (!pred({key, json}))
      continue;

    if (m_cacheValuesCountLimit <= m_values.size())
      m_values.emplace(key, CopyJsonString(value));
    else
      m_values.emplace(key, std::move(json));
  }

  m_storage.clear();
}

// static
bool KeyValueStorage::ParseKeyValueLine(std::string const & line, std::streamoff lineNumber,
    uint64_t & key, std::string & value)
{
  auto const pos = line.find(" ");
  if (pos == std::string::npos)
  {
    LOG(LWARNING, ("Cannot find separator in line", lineNumber));
    return false;
  }

  int64_t id;
  if (!strings::to_int64(line.substr(0, pos), id))
  {
    LOG(LWARNING, ("Cannot parse id", line.substr(0, pos) , "in line", lineNumber));
    return false;
  }

  key = static_cast<uint64_t>(id);
  value = line.c_str() + pos + 1;
  return true;
}

void KeyValueStorage::Insert(uint64_t key, JsonString && valueJson, base::JSONPtr && value)
{
  CHECK(valueJson.get(), ());

  auto json = valueJson.get(); // value usage after std::move(valueJson)

  auto emplace = value && m_values.size() < m_cacheValuesCountLimit
                    ? m_values.emplace(key, std::make_shared<JsonValue>(std::move(value)))
                    : m_values.emplace(key, std::move(valueJson));
  if (!emplace.second) // it is ok for OSM relation with several outer borders
    return;

  m_storage << static_cast<int64_t>(key) << " " << json << "\n";
}

std::shared_ptr<JsonValue> KeyValueStorage::Find(uint64_t key) const
{
  auto const it = m_values.find(key);
  if (it == std::end(m_values))
    return {};

  if (auto json = boost::get<std::shared_ptr<JsonValue>>(&it->second))
    return *json;

  auto const & jsonString = boost::get<JsonString>(it->second);
  auto json = std::make_shared<JsonValue>(json_loads(jsonString.get(), 0, nullptr));
  CHECK(json, ());
  return json;
}
  
size_t KeyValueStorage::Size() const
{
  return m_values.size();
}

KeyValueStorage::JsonString KeyValueStorage::CopyJsonString(std::string const & value) const
{
  char * copy = static_cast<char *>(std::malloc(value.size() + 1));
  std::strncpy(copy, value.data(), value.size() + 1);
  return JsonString{copy};
}
}  // namespace generator
