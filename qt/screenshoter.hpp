#pragma once

#include "storage/storage_defines.hpp"

#include <QtWidgets/QWidget>

#include <list>
#include <string>

class Framework;

class Screenshoter
{
public:
  Screenshoter(Framework & framework, QWidget * widget);

  void ProcessBookmarks(std::string const & dir);

  void PrepareCountries();
  void OnCountryStatusChanged(storage::CountryId countryId);

private:
  void ProcessNextKml();
  void DoScreenshot();

  Framework & m_framework;
  QWidget * m_widget;
  std::list<std::string> m_filesToProcess;
  std::set<storage::CountryId> countriesToDownload;
  std::string m_nextScreenshotName;
};
