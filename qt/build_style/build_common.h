#pragma once

#include "std/string.hpp"
#include "std/utility.hpp"

#include <QString>
#include <QProcessEnvironment>

inline string to_string(const QString & qs) { return qs.toUtf8().constData(); }

pair<int, QString> ExecProcess(QString const & cmd, QProcessEnvironment const * env = nullptr);

bool CopyFile(QString const & oldFile, QString const & newFile);
