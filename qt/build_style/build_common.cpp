#include "build_common.h"

#include <QDir>
#include <QFile>
#include <QProcess>

pair<int, QString> ExecProcess(QString const & cmd, QProcessEnvironment const * env)
{
  QProcess p;
  if (nullptr != env)
    p.setProcessEnvironment(*env);
  p.start(cmd);
  p.waitForFinished(-1);

  int const exitCode = p.exitCode();

  QString output = p.readAllStandardOutput();
  QString const error = p.readAllStandardError();
  if (!error.isEmpty())
  {
    if (!output.isEmpty())
      output += "\n";
    output += error;
  }

  return make_pair(exitCode, output);
}

bool CopyFile(QString const & oldFile, QString const & newFile)
{
  if (!QFile::exists(oldFile))
    return false;
  if (QFile::exists(newFile) && !QFile::remove(newFile))
    return false;
  return QFile::copy(oldFile, newFile);
}

QString JoinFoldersToPath(std::initializer_list<QString> const & folders)
{
  QString result;
  for (auto it = folders.begin(); it != folders.end(); ++it)
  {
    if (it != folders.begin())
      result.append(QDir::separator());
    result.append(*it);
  }
  return QDir::cleanPath(result);
}
