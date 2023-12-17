#pragma once

#include <map>
#include <mutex>
#include <string>

namespace platform
{
class StringStorageBase
{
public:
  explicit StringStorageBase(std::string const & path);
  void Save() const;
  void Clear();
  bool GetValue(std::string const & key, std::string & outValue) const;
  void SetValue(std::string const & key, std::string && value);
  void Update(std::map<std::string, std::string> const & values);
  void DeleteKeyAndValue(std::string const & key);
  
private:
  using Container = std::map<std::string, std::string>;
  Container m_values;
  mutable std::mutex m_mutex;
  std::string const m_path;
};
}  // namespace platform
