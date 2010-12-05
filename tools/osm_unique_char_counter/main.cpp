#include "../../indexer/xmlparser.h"
#include <iostream>
#include <cmath>
#include <fstream>

#include <QtCore/QString>

using namespace std;

template <class TDoClass>
class XMLDispatcher
{
public:
  XMLDispatcher(TDoClass & doClass) : m_fire(false), m_k_ok(false), m_doClass(doClass) {}
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
  void Process() {}
  void Pop()
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
  bool m_fire;
  bool m_k_ok;
  string m_v;
  TDoClass & m_doClass;
};

static int const KMaxXMLFileBufferSize = 65536;

static size_t gLobalCounter = 0;

template <typename XMLDispatcherT>
bool ParseXML(XMLDispatcherT & dispatcher)
{
  // Create the parser
  XmlParser<XMLDispatcherT> parser(dispatcher);
  if (!parser.Create()) return false;

  double progress = 0.;
  int const multiplier = 100;
  double const step = multiplier * 1024 * 1024;
  double next_progress = progress + step; 
  size_t mb = 0;
  while (!cin.eof())
  {
    char * buffer = static_cast<char *>(parser.GetBuffer(KMaxXMLFileBufferSize));
    if (buffer == 0)
      return false;

    cin.read(buffer, KMaxXMLFileBufferSize);
    progress += KMaxXMLFileBufferSize;

    if (progress > next_progress)
    {
      mb += multiplier;
      cout << mb << " Mb (" << gLobalCounter << ")" << endl;
      next_progress += step;
    }

    if (!parser.ParseBuffer(cin.gcount(), cin.eof()))
      return false;
  }

  return true;
}

struct Counter
{
  Counter()
  {
    m_size = pow(double(2.), int(sizeof(ushort)*8));
    m_array = new long double[m_size];
    fill(m_array, m_array + m_size, (long double)(0.));
  }
  ~Counter()
  {
    delete[] m_array;
  }
  void operator()(string const & utf8)
  {
    QString s(QString::fromUtf8(utf8.c_str(), utf8.size()));
    for (int i = 0; i < s.size(); ++i)
    {
      ushort code = s[i].unicode();
      if (m_array[code] == 0)
      {
        ++gLobalCounter;
        cout << code << " (" << gLobalCounter << ")" << endl;
      }
      m_array[code] += 1.;
    }
  }

  void PrintResult()
  {
    ofstream file("results.txt");

    cout << endl << "RESULTS:" << endl;
    cout << "Code" << "\t" << "Count" << endl;
    file << "Code\tCount\t#\tSymbol" << endl;
    cout << "=========================================================" << endl;
    file << "=========================================================" << endl;
    for (size_t i = 0; i < m_size; ++i)
    {
      if (m_array[i] != 0.)
      {
        cout << i << "\t" << m_array[i] << endl;
        file << i << "\t" << m_array[i] << endl;
      }
    }

    cout << endl << "Total symbols: " << gLobalCounter << endl;
    file << endl << "Total symbols: " << gLobalCounter << endl;
  }

  long double * m_array;
  size_t m_size;
};

int main(int argc, char *argv[])
{
  Counter c;
  XMLDispatcher<Counter> dispatcher(c);
  ParseXML(dispatcher);
  c.PrintResult();
  
  return 0;
}
