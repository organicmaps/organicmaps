#pragma once
#include "logging.h"
#include "pagedownloader.h"
#include "stringparser.h"

#include <QObject>
#include <vector>


class MainManager : public QObject
{
  Q_OBJECT

  Logging m_log;
  PageDownloader m_downloader;
  ContentParser m_parser;

  QString m_outDir;

  class Country
  {
    std::vector<QString> m_codes;
    std::vector<size_t> m_langUrls;

  public:
    QString m_name;

    Country(char const * name) : m_name(name) {}

    void AddCode(QString const & code);
    void AddUrl(size_t url);

    bool GetResult(QString & res, MainManager const & m) const;
  };

  std::vector<Country> m_countries;
  std::vector<QString> m_langUrls;

  size_t m_index;

public:
  MainManager(QString const & outDir);

  void ProcessCountryList(QString const & file);

protected:
  void ProcessNextCountry();
  void ProcessLangEntry(QString const & xml, QString const & entry);

public:   // need for functor
  char const * LangNameToCode(QString const & name);
  void AppendResult(QString const & code);
protected:
  void AppendLangUrl(QString url);

  void ProcessNextLanguage();
  bool ProcessCodeEntry(QString const & xml, QString const & entry);

  void CreateResultFiles();

private slots:
  void countryDownloaded(QString const & s);
  void languageDownloaded(QString const & s);
};
