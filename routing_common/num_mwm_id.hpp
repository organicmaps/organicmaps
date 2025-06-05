#pragma once

#include "platform/country_file.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <vector>

namespace routing
{
using NumMwmId = std::uint16_t;
NumMwmId constexpr kFakeNumMwmId = std::numeric_limits<NumMwmId>::max();
NumMwmId constexpr kGeneratorMwmId = 0;

class NumMwmIds final
{
public:
  bool IsEmpty() const { return m_idToFile.empty(); }

  void RegisterFile(platform::CountryFile const & file)
  {
    if (ContainsFile(file))
      return;

    NumMwmId const id = base::asserted_cast<NumMwmId>(m_idToFile.size());
    m_idToFile.push_back(file);
    m_fileToId[file] = id;

    // LOG(LDEBUG, ("MWM:", file.GetName(), "=", id));
  }

  bool ContainsFile(platform::CountryFile const & file) const { return m_fileToId.find(file) != m_fileToId.cend(); }

  bool ContainsFileForMwm(NumMwmId mwmId) const { return mwmId < m_idToFile.size(); }

  platform::CountryFile const & GetFile(NumMwmId mwmId) const
  {
    ASSERT_LESS(mwmId, m_idToFile.size(), ());
    return m_idToFile[mwmId];
  }

  NumMwmId GetId(platform::CountryFile const & file) const
  {
    auto const it = m_fileToId.find(file);
    ASSERT(it != m_fileToId.cend(), ("Can't find mwm id for", file));
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
