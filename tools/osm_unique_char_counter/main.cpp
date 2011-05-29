#include "../../base/string_utils.hpp"

#include "../../coding/parse_xml.hpp"

#include "../../std/iostream.hpp"
#include "../../std/fstream.hpp"
#include "../../std/unordered_map.hpp"
#include "../../std/vector.hpp"

#include <locale>
#include <iomanip>

using namespace std;

template <class TDoClass>
class XMLDispatcher
{
  bool m_fire;
  bool m_k_ok;
  string m_v;
  TDoClass & m_doClass;

public:
  XMLDispatcher(TDoClass & doClass) : m_fire(false), m_k_ok(false), m_doClass(doClass) {}

  void CharData(string const &) {}

  bool Push(string const & element)
  {
    if (element == "tag")
      m_fire = true;
    return true;
  }
  void AddAttr(string const & attribute, string const & value)
  {
    if (m_fire)
    {
      if (attribute == "k" && value.find("name") == 0)
        m_k_ok = true;
      else if (attribute == "v")
        m_v = value;
    }
  }
  void Pop(string const &)
  {
    if (m_fire)
    {
      if (m_k_ok)
      {
        m_doClass(m_v);

        m_k_ok = false;
      }
      m_fire = false;
    }
  }
};

typedef unordered_map<strings::UniChar, uint64_t> CountContT;
typedef pair<strings::UniChar, uint64_t> ElemT;
typedef unordered_map<strings::UniChar, string> UniMapT;

bool SortFunc(ElemT const & e1, ElemT const & e2)
{
  return e1.second > e2.second;
}

struct Counter
{
  CountContT m_counter;
  UniMapT & m_uni;

  Counter(UniMapT & uni) : m_uni(uni) {}

  void operator()(string const & utf8s)
  {
    strings::UniString us;
    utf8::unchecked::utf8to32(utf8s.begin(), utf8s.end(), back_inserter(us));
    for (strings::UniString::iterator it = us.begin(); it != us.end(); ++it)
    {
      pair<CountContT::iterator, bool> found = m_counter.insert(
            make_pair(*it, 1));
      if (!found.second)
        ++found.first->second;
    }
  }

  void PrintResult()
  {
    // sort
    typedef vector<ElemT> SortVecT;
    SortVecT v(m_counter.begin(), m_counter.end());
    sort(v.begin(), v.end(), SortFunc);


    locale loc("en_US.UTF-8");
    cout.imbue(loc);

    string c;
    c.resize(10);
    for (size_t i = 0; i < v.size(); ++i)
    {
      c.clear();
      utf8::unchecked::append(v[i].first, back_inserter(c));
      UniMapT::iterator found = m_uni.find(v[i].first);
      if (found == m_uni.end())
        cout << dec << v[i].second << " " << hex << v[i].first << " " << c << endl;
      else
        cout << dec << v[i].second << " "  << c << " " << found->second << endl;
    }
  }
};

struct StdinReader
{
  size_t Read(char * buffer, size_t bufferSize)
  {
    return fread(buffer, sizeof(char), bufferSize, stdin);
  }
};

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    cerr << "Usage: " << argv[0] << " PathToUnicodeFile" << endl;
    return -1;
  }

  // load unicodedata.txt file
  ifstream f(argv[1]);
  if (!f.good())
  {
    cerr << "Can't open unicodedata.txt file " << argv[1] << endl;
    return -1;
  }

  UniMapT m;

  string line;
  while (f.good())
  {
    getline(f, line);
    size_t const semic = line.find(';');
    if (semic == string::npos)
      continue;
    istringstream stream(line.substr(0, semic));
    strings::UniChar c;
    stream >> hex >> c;
    m[c] = line;
  }

  Counter c(m);
  XMLDispatcher<Counter> dispatcher(c);
  StdinReader reader;
  ParseXML(reader, dispatcher);
  c.PrintResult();

  return 0;
}
