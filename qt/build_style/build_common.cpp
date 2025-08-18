#include "build_common.h"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>

#include <exception>
#include <iomanip>  // std::quoted
#include <regex>
#include <string>

QString ExecProcess(QString const & program, std::initializer_list<QString> args, QProcessEnvironment const * env)
{
  // Quote all arguments.
  QStringList qargs(args);
  for (auto i = qargs.begin(); i != qargs.end(); ++i)
    *i = "\"" + *i + "\"";

  QProcess p;
  if (nullptr != env)
    p.setProcessEnvironment(*env);

  p.start(program, qargs, QIODevice::ReadOnly);
  p.waitForFinished(-1);

  int const exitCode = p.exitCode();
  QString output = p.readAllStandardOutput();
  QString const error = p.readAllStandardError();
  if (exitCode != 0)
  {
    QString msg = "Error: " + program + " " + qargs.join(" ") + "\nReturned " + QString::number(exitCode);
    if (!output.isEmpty())
      msg += "\n" + output;
    if (!error.isEmpty())
      msg += "\nSTDERR:\n" + error;
    throw std::runtime_error(msg.toStdString());
  }
  if (!error.isEmpty())
  {
    QString const msg = "STDERR with a zero exit code:\n" + program + " " + qargs.join(" ");
    throw std::runtime_error(msg.toStdString());
  }
  return output;
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
  if (!CopyFile(JoinPathQt({resourceDir, name}), JoinPathQt({output, name})))
    throw std::runtime_error(std::string("Cannot copy file ") + name.toStdString() + " to " + output.toStdString());
}

void CopyToResources(QString const & name, QString const & input, QString const & newName)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  if (!CopyFile(JoinPathQt({input, name}), JoinPathQt({resourceDir, newName.isEmpty() ? name : newName})))
    throw std::runtime_error(std::string("Cannot copy file ") + name.toStdString() + " from " + input.toStdString());
}

QString JoinPathQt(std::initializer_list<QString> folders)
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

QString GetExternalPath(QString const & name, QString const & primaryPath, QString const & secondaryPath)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString path = JoinPathQt({resourceDir, primaryPath, name});
  if (!QFileInfo::exists(path))
    path = JoinPathQt({resourceDir, secondaryPath, name});

  // Special case for looking for in application folder.
  if (!QFileInfo::exists(path) && secondaryPath.isEmpty())
  {
    std::string const appPath = QCoreApplication::applicationDirPath().toStdString();

    std::regex re("(/[^/]*\\.app)");
    std::smatch m;
    if (std::regex_search(appPath, m, re) && m.size() > 0)
      path.fromStdString(base::JoinPath(m[0], name.toStdString()));
  }
  return path;
}

QString GetProtobufEggPath()
{
  return GetExternalPath("protobuf-3.3.0-py2.7.egg", "kothic", "../3party/protobuf");
}
