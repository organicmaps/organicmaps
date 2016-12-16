#include "base/string_format.hpp"

#include <list>


namespace strings
{
  std::string const FormatImpl(std::string const & s, std::string arr[], size_t count)
  {
    size_t offs = 0;
    std::list<size_t> fieldOffs;

    std::string res = s;

    while (true)
    {
      offs = res.find("^", offs);
      if (offs == std::string::npos)
        break;
      else
      {
        if ((offs != 0) && (res[offs - 1] == '\\'))
        {
          res = res.erase(offs - 1, 1);
          --offs;
        }
        else
          fieldOffs.push_back(offs);

        ++offs;
      }
    }

    offs = 0;
    size_t i = 0;

    for (std::list<size_t>::const_iterator offsIt = fieldOffs.begin();
        (offsIt != fieldOffs.end()) && (i < count);
        ++offsIt, ++i)
    {
      res.replace(*offsIt + offs, 1, arr[i]);
      offs += (arr[i].size() - 1);
    }

    return res;
  }
}
