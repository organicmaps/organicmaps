#include "logging.h"

#include <iostream>


using namespace std;

Logging::Logging()
{
}

void Logging::Print(STATUS s, QString const & msg)
{
  cout << StatusToString(s).toStdString() << " " << msg.toStdString() << endl;
}

void Logging::Percent(qint64 curr, qint64 total)
{
}

QString Logging::StatusToString(STATUS s) const
{
  switch (s)
  {
  case INFO: return "INFO";
  case WARNING: return "WARNING";
  case ERROR: return "ERROR";
  default: return "NONE";
  }
}
