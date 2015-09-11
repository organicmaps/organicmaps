#include "mainmanager.h"

#include <QFile>

#include <algorithm>
#include <fstream>
#include <string>


using namespace std;

void MainManager::Country::AddCode(QString const & code)
{
  if (m_codes.end() == find(m_codes.begin(), m_codes.end(), code))
    m_codes.push_back(code);
}

void MainManager::Country::AddUrl(size_t url)
{
  if (m_langUrls.end() == find(m_langUrls.begin(), m_langUrls.end(), url))
    m_langUrls.push_back(url);
}

namespace
{
  void append(QString & res, QString const & s)
  {
    if (res.isEmpty()) res = s;
    else res = res + "|" + s;
  }
}

bool MainManager::Country::GetResult(QString & res, MainManager const & m) const
{
  res.clear();

  for (size_t i = 0; i < m_codes.size(); ++i)
    append(res, m_codes[i]);

  for (size_t i = 0; i < m_langUrls.size(); ++i)
  {
    QString const code = m.m_langUrls[m_langUrls[i]];
    if (!code.isEmpty())
      append(res, code);
  }

  return !res.isEmpty();
}


MainManager::MainManager(QString const & outDir)
  : m_downloader(m_log), m_parser(m_log), m_outDir(outDir)
{
}

char const * MainManager::LangNameToCode(QString const & name)
{
  if (name.contains("English", Qt::CaseInsensitive)) return "en";
  if (name.contains("Spanish", Qt::CaseInsensitive)) return "es";
  if (name.contains("French", Qt::CaseInsensitive)) return "fr";
  if (name.contains("Mandarin", Qt::CaseInsensitive)) return "zh";
  return 0;
}

void MainManager::ProcessCountryList(QString const & file)
{
  ifstream s(file.toStdString().c_str());
  if (!s.is_open() || !s.good())
  {
    m_log.Print(Logging::ERROR, QString("Can't open file: ") + file);
    return;
  }

  char buffer[256];
  while (s.good())
  {
    s.getline(buffer, 256);
    if (strlen(buffer) > 0)
      m_countries.push_back(buffer);
  }

  m_downloader.ConnectFinished(this, SLOT(countryDownloaded(QString const &)));

  m_index = 0;
  ProcessNextCountry();
}

namespace
{
  void get_country_url(QString & name)
  {
    int const i = name.indexOf('_');
    if (i != -1)
      name = name.mid(0, i);    // for regions return country name

    name.replace(' ', '_');   // make correct wiki url
  }
}

void MainManager::ProcessNextCountry()
{
  if (m_index >= m_countries.size())
  {
    m_downloader.ConnectFinished(this, SLOT(languageDownloaded(QString const &)));

    m_index = 0;
    ProcessNextLanguage();
    return;
  }

  QString url = m_countries[m_index].m_name;
  get_country_url(url);

  m_downloader.Download(QString("http://en.wikipedia.org/wiki/") + url);
}

namespace
{
  class append_result
  {
    MainManager & m_manager;
  public:
    append_result(MainManager & m) : m_manager(m) {}
    void operator() (QString const & s)
    {
      char const * code = m_manager.LangNameToCode(s);
      if (code)
        m_manager.AppendResult(code);
    }
  };

  class nodes_iterator
  {
    QDomElement m_node;
    bool m_isList;

  public:
    nodes_iterator(QDomElement const & root) : m_isList(false)
    {
      // process single elements ...
      m_node = root.firstChildElement("a");
      if (m_node.isNull())
      {
        // ... or compound list
        m_node = root.firstChildElement("ul");
        if (!m_node.isNull())
        {
          m_node = m_node.firstChildElement("li");
          m_isList = true;
        }
      }
    }

    bool valid() const { return !m_node.isNull(); }

    QDomElement get() const
    {
      return (m_isList ? m_node.firstChildElement("a") : m_node);
    }

    void next()
    {
      m_node = m_node.nextSiblingElement(m_isList ? "li" : "a");
    }
  };
}

void MainManager::ProcessLangEntry(QString const & xml, QString const & entry)
{
  if (m_parser.InitSubDOM(xml, entry, "td"))
  {
    nodes_iterator it(m_parser.Root());

    if (!it.valid())
    {
      // try to get language from root node
      TokenizeString(m_parser.Root().text(), ", ", append_result(*this));
    }

    // iterate through child nodes
    while (it.valid())
    {
      QDomElement e = it.get();

      char const * code = LangNameToCode(e.text());
      if (code)
      {
        AppendResult(code);
      }
      else
      {
        QString const url = e.attribute("href");
        if (!url.isEmpty())
          AppendLangUrl(url);
        else
          m_log.Print(Logging::WARNING, QString("Undefined language without url: ") + e.text());
      }

      it.next();
    }
  }
}

void MainManager::countryDownloaded(QString const & s)
{
  ProcessLangEntry(s, "Official language(s)");
  ProcessLangEntry(s, "National language");

  ++m_index;
  ProcessNextCountry();
}

void MainManager::AppendResult(QString const & code)
{
  m_countries[m_index].AddCode(code);
}

void MainManager::AppendLangUrl(QString url)
{
  {
    int const i = url.lastIndexOf("/");
    if (i != -1)
      url = url.mid(i+1);
  }

  size_t index;
  {
    vector<QString>::iterator i = find(m_langUrls.begin(), m_langUrls.end(), url);
    if (i == m_langUrls.end())
    {
      m_langUrls.push_back(url);
      index = m_langUrls.size()-1;
    }
    else
      index = std::distance(m_langUrls.begin(), i);
  }

  m_countries[m_index].AddUrl(index);
}

void MainManager::ProcessNextLanguage()
{
  if (m_index >= m_langUrls.size())
  {
    CreateResultFiles();
    m_log.Print(Logging::INFO, "Done!");

    exit(0);
    return;
  }

  m_downloader.Download(QString("http://en.wikipedia.org/wiki/") + m_langUrls[m_index]);
}

bool MainManager::ProcessCodeEntry(QString const & xml, QString const & entry)
{
  if (m_parser.InitSubDOM(xml, entry, "td"))
  {
    QDomElement e = m_parser.Root().firstChildElement("tt");
    if (!e.isNull())
    {
      QString const name = e.text();
      if (!name.isEmpty())
      {
        m_langUrls[m_index] = name;
        return true;
      }
    }
  }

  return false;
}

void MainManager::languageDownloaded(QString const & s)
{
  if (!ProcessCodeEntry(s, "ISO 639-1"))
    if (!ProcessCodeEntry(s, "ISO 639-2"))
      if (!ProcessCodeEntry(s, "ISO 639-3"))
      {
        m_log.Print(Logging::WARNING, QString("Can't find code for url: ") + m_langUrls[m_index]);
        m_langUrls[m_index] = QString();
      }

  ++m_index;
  ProcessNextLanguage();
}

void MainManager::CreateResultFiles()
{
  m_log.Print(Logging::INFO, "Results:");

  for (size_t i = 0; i < m_countries.size(); ++i)
  {
    QString s;
    if (m_countries[i].GetResult(s, *this))
    {
      QFile f(m_outDir + m_countries[i].m_name + QString(".meta"));
      f.open(QFile::WriteOnly);
      f.write(s.toStdString().c_str());
    }
    else
      m_log.Print(Logging::WARNING, QString("No languages for country: ") + m_countries[i].m_name);
  }
}
