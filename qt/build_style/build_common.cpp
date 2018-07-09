#include "build_common.h"

#include "platform/platform.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>

#include <exception>

std::pair<int, QString> ExecProcess(QString const & cmd, QProcessEnvironment const * env)
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

  return std::make_pair(exitCode, output);
}

bool CopyFile(QString const & oldFile, QString const & newFile)
{
  if (oldFile == newFile)
    return true;
  if (!QFile::exists(oldFile))
    return false;
  if (QFile::exists(newFile) && !QFile::remove(newFile))
    return false;
  return QFile::copy(oldFile, newFile);
}

void CopyFromResources(QString const & name, QString const & output)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  if (!CopyFile(JoinFoldersToPath({resourceDir, name}),
                JoinFoldersToPath({output, name})))
  {
    throw std::runtime_error(std::string("Cannot copy file ") +
                             name.toStdString() +
                             " to " + output.toStdString());
  }
}

void CopyToResources(QString const & name, QString const & input, QString const & newName)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  if (!CopyFile(JoinFoldersToPath({input, name}),
                JoinFoldersToPath({resourceDir, newName.isEmpty() ? name : newName})))
  {
    throw std::runtime_error(std::string("Cannot copy file ") +
                             name.toStdString() +
                             " from " + input.toStdString());
  }
}

QString JoinFoldersToPath(std::initializer_list<QString> const & folders)
{
  QString result;
  bool firstInserted = false;
  for (auto it = folders.begin(); it != folders.end(); ++it)
  {
    if (it->isEmpty() || *it == QDir::separator())
      continue;

    if (firstInserted)
      result.append(QDir::separator());

    result.append(*it);
    firstInserted = true;
  }
  return QDir::cleanPath(result);
}

QString GetExternalPath(QString const & name, QString const & primaryPath,
                        QString const & secondaryPath)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString path = JoinFoldersToPath({resourceDir, primaryPath, name});
  if (!QFileInfo::exists(path))
    path = JoinFoldersToPath({resourceDir, secondaryPath, name});

  // Special case for looking for in application folder.
  if (!QFileInfo::exists(path) && secondaryPath.isEmpty())
  {
    QString const appPath = QCoreApplication::applicationDirPath();
    QRegExp rx("(/[^/]*\\.app)", Qt::CaseInsensitive);
    int i = rx.indexIn(appPath);
    if (i >= 0)
      path = JoinFoldersToPath({appPath.left(i), name});
  }
  return path;
}

QString GetProtobufEggPath()
{
  return GetExternalPath("protobuf-3.3.0-py2.7.egg", "kothic", "../3party/protobuf");
}
