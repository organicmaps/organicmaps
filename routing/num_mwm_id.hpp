#pragma once

#include "platform/country_file.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <vector>

namespace routing
{
using NumMwmId = std::uint16_t;
NumMwmId constexpr kFakeNumMwmId = std::numeric_limits<NumMwmId>::max();

class NumMwmIds final
{
public:
  void RegisterFile(platform::CountryFile const & file)
  {
    if (ContainsFile(file))
      return;

    NumMwmId const id = base::asserted_cast<NumMwmId>(m_idToFile.size());
    m_idToFile.push_back(file);
    m_fileToId[file] = id;
  }

  bool ContainsFile(platform::CountryFile const & file) const
  {
    return m_fileToId.find(file) != m_fileToId.cend();
  }

  platform::CountryFile const & GetFile(NumMwmId mwmId) const
  {
    size_t const index = base::asserted_cast<size_t>(mwmId);
    CHECK_LESS(index, m_idToFile.size(), ());
    return m_idToFile[index];
  }

  NumMwmId GetId(platform::CountryFile const & file) const
  {
    auto const it = m_fileToId.find(file);
    CHECK(it != m_fileToId.cend(), ("Can't find mwm id for", file));
    return it->second;
  }

  template <typename F>
  void ForEachId(F && f) const
  {
    for (NumMwmId id = 0; id < base::asserted_cast<NumMwmId>(m_idToFile.size()); ++id)
      f(id);
  }

private:
  std::vector<platform::CountryFile> m_idToFile;
  std::map<platform::CountryFile, NumMwmId> m_fileToId;
};
}  //  namespace routing
