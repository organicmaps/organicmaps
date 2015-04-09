#include "coding/reader.hpp"

#include "base/string_utils.hpp"


void Reader::ReadAsString(string & s) const
{
  s.clear();
  size_t const sz = static_cast<size_t>(Size());
  s.resize(sz);
  Read(0, &s[0], sz);
}

bool Reader::IsEqual(string const & name1, string const & name2)
{
#if defined(OMIM_OS_WINDOWS)
  return strings::EqualNoCase(name1, name2);
#else
  return (name1 == name2);
#endif
}

namespace
{
  bool AssertPosAndSizeImpl(uint64_t pos, uint64_t size, uint64_t readerSize)
  {
    bool const ret1 = (pos + size <= readerSize);
    bool const ret2 = (size <= static_cast<size_t>(-1));
    ASSERT ( ret1 && ret2, (pos, size, readerSize) );
    return (ret1 && ret2);
  }
}

bool MemReader::AssertPosAndSize(uint64_t pos, uint64_t size) const
{
  return AssertPosAndSizeImpl(pos, size, Size());
}

bool SharedMemReader::AssertPosAndSize(uint64_t pos, uint64_t size) const
{
  return AssertPosAndSizeImpl(pos, size, Size());
}
