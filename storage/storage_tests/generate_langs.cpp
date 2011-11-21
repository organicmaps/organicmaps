#include "../../testing/testing.hpp"

#include "../country.hpp"

#include "../../platform/platform.hpp"

#include "../../coding/file_reader.hpp"
#include "../../coding/parse_xml.hpp"

#include "../../base/string_utils.hpp"

#include "../../std/fstream.hpp"


/*
namespace
{
  class LangXMLGetter
  {
    string m_path;
    multimap<string, string> & m_code2file;

    int m_state;
    string m_country, m_lang;
    double m_percent;
    bool m_official;

    string m_res;

  public:
    LangXMLGetter(string const & path, multimap<string, string> & code2file)
      : m_path(path), m_code2file(code2file), m_state(0)
    {
    }

    bool Push(string const & name)
    {
      if (m_state == 0 && name == "territoryInfo")
      {
        m_state = 1;
      }
      else if (m_state == 1 && name == "territory")
      {
        m_country.clear();
        m_res.clear();

        m_state = 2;
      }
      else if (m_state == 2 && name == "languagePopulation")
      {
        m_lang.clear();
        m_percent = 0.0;
        m_official = false;

        m_state = 3;
      }

      return true;
    }

    void AddAttr(string const & name, string const & value)
    {
      switch (m_state)
      {
      case 2:
        if (name == "type")
          m_country = value;
        break;
      case 3:
        if (name == "type")
          m_lang = value;
        else if (name == "populationPercent")
          strings::to_double(value, m_percent);
        else
        {
          if (name == "officialStatus" &&
              (value == "official" || value == "de_facto_official"))
          {
            m_official = true;
          }
        }
      }
    }

    void Pop(string const &)
    {
      switch (m_state)
      {
      case 3:
        // emit language
        if (!m_lang.empty() && (m_percent >= 10.0 || m_official))
        {
          if (m_res.empty()) m_res = m_lang;
          else m_res = m_res + "|" + m_lang;
        }
        break;

      case 2:
        // save result languages
        if (!m_country.empty() && !m_res.empty())
        {
          typedef multimap<string, string>::const_iterator iter_t;

          strings::MakeLowerCase(m_country);
          pair<iter_t, iter_t> r = m_code2file.equal_range(m_country);
          while (r.first != r.second)
          {
            ofstream file((m_path + r.first->second + ".meta").c_str());
            file << m_res;

            ++r.first;
          }
        }
        break;
      }

      if (m_state > 0) --m_state;
    }

    void CharData(string const &) {}
  };
}

UNIT_TEST(GenerateLanguages)
{
  string buffer;
  ReaderPtr<Reader>(GetPlatform().GetReader(COUNTRIES_FILE)).ReadAsString(buffer);

  multimap<string, string> code2file;
  storage::LoadCountryCode2File(buffer, code2file);

  string const path = "/Users/alena/omim/omim/data/metainfo_test/";
  FileReader reader(path + "supplementalData.xml");

  ReaderSource<FileReader> src(reader);
  LangXMLGetter parser(path, code2file);
  ParseXML(src, parser);
}
*/
