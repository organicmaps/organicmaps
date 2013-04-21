#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <map>
#include <stdint.h>
#include <set>

#include "../../3party/utfcpp/source/utf8.h"

using namespace std;

// get substrings from s divided by delim and pass them to toDo including empty strings
template <class ToDo> void TokenizeString(string const & s, char const * delim, ToDo & toDo)
{
  size_t const count = s.size();
  size_t i = 0;
  while (i <= count)
  {
    size_t e = s.find_first_of(delim, i);
    if (e == string::npos)
      e = count;

    toDo(s.substr(i, e-i));

    i = e + 1;
  }
}

struct Collector
{
  vector<string> m_strings;
  void operator()(string const & s)
  {
    m_strings.push_back(s);
  }
};

typedef vector<uint32_t> DecompositionT;

struct Symbol
{
  uint32_t m_code;
  string m_name;
  string m_category;
  DecompositionT m_decomposed;

  bool ParseLine(string const & line)
  {
    Collector c;
    TokenizeString(line, ";", c);
    if (c.m_strings.size() != 15)
    {
      cerr << "Invalid line? Less than 15 ';': " << line << endl;
      return false;
    }
    if (c.m_strings[0].empty())
    {
      cerr << "Invalid line? Code is empty" << endl;
      return false;
    }
    if (c.m_strings[2].empty())
    {
      cerr << "Invalid line? General category is empty" << endl;
      return false;
    }

    { // Symbol code
      istringstream stream(c.m_strings[0]);
      stream >> hex >> m_code;
    }

    { // Name
      m_name = c.m_strings[1];
    }

    { // General category
      m_category = c.m_strings[2];
    }

    // optional decomposition codes
    if (!c.m_strings[5].empty())
    {
      Collector codes;
      TokenizeString(c.m_strings[5], " ", codes);
      for (size_t i = 0; i < codes.m_strings.size(); ++i)
      {
        istringstream stream(codes.m_strings[i]);
        uint32_t c;
        stream >> hex >> c;
        if (c != 0)
          m_decomposed.push_back(c);
      }
    }
    return true;
  }
};

typedef map<uint32_t, Symbol> SymbolsMapT;

bool IsCombiningMarkCategory(string const & cat)
{
  return cat == "Mn" || cat == "Mc" || cat == "Me";
}

bool IsUpperOrTitleCase(string const & cat)
{
  return cat == "Lu" || cat == "Lt";
}

vector<Symbol> GetDecomposedSymbols(Symbol const & s, SymbolsMapT const & allChars)
{
  vector<Symbol> results;
  if (s.m_decomposed.empty() && !IsCombiningMarkCategory(s.m_category))
  {
    results.push_back(s);
  }
  else
  {
    for (size_t i = 0; i < s.m_decomposed.size(); ++i)
    {
      if (!IsCombiningMarkCategory(s.m_category))
      {
        SymbolsMapT::const_iterator found = allChars.find(s.m_decomposed[i]);
        if (found != allChars.end())
        {
          vector<Symbol> r = GetDecomposedSymbols(found->second, allChars);
          results.insert(results.end(), r.begin(), r.end());
        }
      }
    }
  }
  return results;
}

string SymbolToString(uint32_t s)
{
  string res;
  utf8::unchecked::append(s, back_inserter(res));
  return res;
}

template <class T>
void ForEachNormChar(SymbolsMapT const & allChars, T & f)
{
  /// Umlauts -> no umlauts matching
  for (SymbolsMapT::const_iterator it = allChars.begin(); it != allChars.end(); ++it)
  {
    /// process only chars which can be decomposed
    if (!it->second.m_decomposed.empty() && !IsCombiningMarkCategory(it->second.m_category))
    {
      vector<Symbol> r = GetDecomposedSymbols(it->second, allChars);
      if (!r.empty())
      {
        DecompositionT d;
        for (size_t i = 0; i < r.size(); ++i)
          d.push_back(r[i].m_code);
        f(it->first, d);
      }
    }
  }
}

typedef vector<DecompositionT> OutVecT;

struct DecomposeCollector
{
  OutVecT & m_v;
  DecomposeCollector(OutVecT & v) : m_v(v) {}
  void operator()(uint32_t code, DecompositionT const & r)
  {
    m_v.push_back(r);
  }
};

bool SortFunc(DecompositionT const & d1, DecompositionT const & d2)
{
  return d1.size() > d2.size();
}

struct Compressor
{
  DecompositionT & m_out;
  Compressor(DecompositionT & out) : m_out(out) {}
  void operator()(DecompositionT const & d)
  {
    if (m_out.size() >= d.size())
    {
      DecompositionT::iterator begIt = m_out.begin();
      DecompositionT::iterator endIt = m_out.begin() + d.size();
      while (endIt != m_out.end())
      {
        DecompositionT tmp(begIt, endIt);
        if (tmp == d)
        { // already compressed
          return;
        }
        ++begIt;
        ++endIt;
      }
      m_out.insert(m_out.end(), d.begin(), d.end());
    }
    else
      m_out.insert(m_out.end(), d.begin(), d.end());
  }
};


typedef map<uint32_t, pair<int, int> > CodeIndexSizeT;
struct IndexAndSizeGenerator
{
  CodeIndexSizeT & m_out;
  DecompositionT const & m_compressed;
  IndexAndSizeGenerator(CodeIndexSizeT & m, DecompositionT const & c)
    : m_out(m), m_compressed(c) {}
  void operator()(uint32_t code, DecompositionT const & r)
  {
    DecompositionT::const_iterator found =
        search(m_compressed.begin(), m_compressed.end(), r.begin(), r.end());
    if (found != m_compressed.end())
      m_out[code] = make_pair(found - m_compressed.begin(), r.size());
    else
      cerr << "WTF??? Not found in compressed sequence " << hex << code << endl;
  }
};

void GenerateNormalization(SymbolsMapT const & allChars)
{
  // first, create an array for all possible replacement combinations
  vector<DecompositionT> decomposes;
  DecomposeCollector collector(decomposes);
  ForEachNormChar(allChars, collector);
  // sort it by number of decompositions
  sort(decomposes.begin(), decomposes.end(), SortFunc);

  // compress decomposition chars by reusing them
  DecompositionT compressed;
  Compressor compressor(compressed);
  for_each(decomposes.begin(), decomposes.end(), compressor);

  // generate indexes and sizes for each char
  CodeIndexSizeT m;
  IndexAndSizeGenerator gen(m, compressed);
  ForEachNormChar(allChars, gen);

//  for (CodeIndexSizeT::iterator it = m.begin(); it != m.end(); ++it)
//  {
//    cout << hex << it->first << " " << dec << it->second.first << " "
//         << dec << it->second.second << endl;
//  }

  // generate code

  // write compressed array
  cout << "#include \"string_utils.hpp\"" << endl << endl;
  cout << "static strings::UniChar const normSymbols[] = {";
  for (size_t i = 0; i < compressed.size(); ++i)
  {
    cout << "0x" << hex << compressed[i];
    if (i != compressed.size() - 1)
      cout << ",";
  }
  cout << "};" << endl << endl;

  // code preamble
  cout << "static void w(strings::UniString & r, uint16_t startIndex, int count)" << endl;
  cout << "{" << endl;
  cout << "  for (int i = 0; i < count; ++i)" << endl;
  cout << "    r.push_back(normSymbols[startIndex + i]);" << endl;
  cout << "}" << endl << endl;

  cout << "void Normalize(strings::UniString & s)" << endl;
  cout << "{" << endl;
  cout << "  strings::UniString r;" << endl;
  cout << "  for (int i = 0; i < s.size(); ++i)" << endl;
  cout << "  {" << endl;
  cout << "    strings::UniChar const c = s[i];" << endl;
  cout << "    // ASCII optimization" << endl;
  cout << "    if (c < 0xa0)" << endl;
  cout << "       r.push_back(c);" << endl;
  cout << "    else" << endl;
  cout << "    {" << endl;
  cout << "      switch (c & 0xffffff00)" << endl;
  cout << "      {" << endl;


  int32_t lastRange = -1;
  for (CodeIndexSizeT::iterator it = m.begin(); it != m.end(); ++it)
  {
    if (lastRange != (it->first & 0xffffff00))
    {
      if (lastRange != -1)
      {
        cout << "          default: r.push_back(c);" << endl;
        cout << "          }" << endl;
        cout << "        }" << endl;
        cout << "        break;" << endl;
      }
      lastRange = it->first & 0xffffff00;

      cout << "      case 0x" << hex << lastRange << ":" << endl;
      cout << "        {" << endl;
      cout << "          switch (static_cast<uint8_t>(c & 0xff))" << endl;
      cout << "          {" << endl;
    }
    cout << "          case 0x" << hex << (it->first & 0xff) << ": w(r,"
         << dec << it->second.first << "," << dec << it->second.second << "); break;" << endl;
  }

  cout << "          default: r.push_back(c);" << endl;
  cout << "          }" << endl;
  cout << "        }" << endl;
  cout << "        break;" << endl;
  cout << "        default: r.push_back(c);" << endl;
  cout << "      }" << endl;
  cout << "    }" << endl;
  cout << "  }" << endl;
  cout << "  s.swap(r);" << endl;
  cout << "}" << endl;


//    /// Umlauts -> no umlauts matching
//    for (SymbolsMapT::iterator it = allChars.begin(); it != allChars.end(); ++it)
//    {
//      /// process only chars which can be decomposed
//      if (!it->second.m_decomposed.empty() && !IsCombiningMarkCategory(it->second.m_category))
//      {
//        vector<Symbol> r = GetDecomposedSymbols(it->second, allChars);
//        if (!r.empty() && r.size() == 1)
//        {
//          if (lastRange != (it->first & 0xffffff00))
//          {
//            lastRange = it->first & 0xffffff00;
//            cout << "};" << endl;
//            cout << maxBytes << endl;
//            cout << "static uint16_t const norm" << hex << lastRange << "[] = {";
//            maxBytes = -1;
//          }
//          int bytes = r[0].m_code > 256*256*256-1 ? 4
//                    : r[0].m_code > 256*256-1 ? 3 : r[0].m_code > 256-1 ? 2 : 1;
//          if (bytes > maxBytes)
//            maxBytes = bytes;
//          cout << "0x" << hex << r[0].m_code << ",";

//  //        pair<mymap::iterator, bool> found = counter.insert(make_pair(r.size(), 1));
//  //        if (!found.second)
//  //          ++found.first->second;

//  //        cout << hex << it->first << " " << SymbolToString(it->first) << " ";
//  //        for (size_t i = 0; i < r.size(); ++i)
//  //          cout << hex << r[i].m_code << " " << SymbolToString(r[i].m_code) << " ";
//  //        cout << endl;
//        }
//      }
//    }
}






int main(int argc, char * argv[])
{
  if (argc < 2)
  {
    cout << "Usage: " << argv[0] << " unicode_text_file" << endl;
    return 0;
  }
  ifstream file(argv[1]);
  string line;

  SymbolsMapT allChars;
  while (file.good())
  {
    getline(file, line);
    Symbol s;
    if (s.ParseLine(line))
      allChars[s.m_code] = s;
  }

  cout << "Loaded " << allChars.size() << " unicode chars" << endl;

  GenerateNormalization(allChars);

  return 0;
}
