#pragma once

#include <QString>

class Logging
{
public:
  Logging();

  enum STATUS { INFO, WARNING, ERROR };

  void Print(STATUS s, QString const & msg);
  void Percent(qint64 curr, qint64 total);

  QString StatusToString(STATUS s) const;
};
