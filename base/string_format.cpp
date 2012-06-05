#include "string_format.hpp"
#include "logging.hpp"

namespace strings
{
  string const FormatImpl(string const & s, list<string> const & l)
  {
    size_t offs = 0;
    list<size_t> fieldOffs;

    string temp = s;

    while (true)
    {
      offs = temp.find("%", offs);
      if (offs == string::npos)
        break;
      else
      {
        if ((offs != 0) && (temp[offs - 1] == '\\'))
        {
          temp = temp.erase(offs - 1, 1);
          --offs;
        }
        else
          fieldOffs.push_back(offs);

        ++offs;
      }
    }

    offs = 0;

    string res = temp;

    list<size_t>::const_iterator offsIt;
    list<string>::const_iterator strIt;

    for (offsIt = fieldOffs.begin(), strIt = l.begin();
        (offsIt != fieldOffs.end()) && (strIt != l.end());
        ++offsIt, ++strIt)
    {
      res.replace(*offsIt + offs, 1, *strIt);
      offs += strIt->size() - 1;
    }

    return res;
  }
}
