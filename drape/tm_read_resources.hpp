#pragma once
#include "platform/platform.hpp"

#include "coding/reader.hpp"
#include "coding/reader_streambuf.hpp"

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace dp::impl
{

template <typename ToDo>
inline void ParsePatternsList(std::string const & patternsFile, ToDo && toDo)
{
  ReaderStreamBuf buffer(GetPlatform().GetReader(patternsFile));
  std::istream is(&buffer);

  std::string line;
  while (std::getline(is, line))
  {
    buffer_vector<double, 8> pattern;
    strings::Tokenize(line, " ", [&](std::string_view token)
    {
      double d = 0.0;
      VERIFY(strings::to_double(token, d), ());
      pattern.push_back(d);
    });

    bool isValid = true;
    for (size_t i = 0; i < pattern.size(); i++)
    {
      if (fabs(pattern[i]) < 1e-5)
      {
        LOG(LWARNING, ("Pattern was skipped", line));
        isValid = false;
        break;
      }
    }

    if (isValid)
      toDo(pattern);
  }
}

}  // namespace dp::impl
