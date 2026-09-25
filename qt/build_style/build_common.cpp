#include "build_common.h"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>

#include <exception>
#include <string>

QString ExecProcess(QString const & program, std::initializer_list<QString> args, QProcessEnvironment const * env)
{
  QStringList const qargs(args);

  QProcess p;
  if (nullptr != env)
    p.setProcessEnvironment(*env);

  LOG(LINFO, ("Running command:", program.toStdString(), "\n   ", qargs.join("\n    ").toStdString()));

  p.start(program, qargs, QIODevice::ReadOnly);
  // A missing binary reports exitCode() == 0, which reads as success below.
  if (!p.waitForStarted(-1))
    throw std::runtime_error("Failed to start " + program.toStdString() + ": " + p.errorString().toStdString());
  p.waitForFinished(-1);

  QString output = p.readAllStandardOutput();
  QString const error = p.readAllStandardError();
  if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0)
  {
    QString msg = "Error: " + program + " " + qargs.join(" ");
    if (p.exitStatus() == QProcess::NormalExit)
      msg += "\nReturned " + QString::number(p.exitCode());
    else
      msg += "\nCrashed: " + p.errorString();
    if (!output.isEmpty())
      msg += "\n" + output;
    if (!error.isEmpty())
      msg += "\nSTDERR:\n" + error;
    throw std::runtime_error(msg.toStdString());
  }
  if (!error.isEmpty())
    LOG(LWARNING, ("Exit code zero, but non-empty STDERR output:", error.toStdString()));
  return output;
}

bool CopyQtFile(QString const & oldFile, QString const & newFile)
{
  if (oldFile == newFile)
    return true;
  if (!QFile::exists(oldFile))
    return false;
  if (QFile::exists(newFile) && !QFile::remove(newFile))
    return false;
  return QFile::copy(oldFile, newFile);
}

void CopyFromDataDir(QString const & name, QString const & destDir)
{
  // The file may live in the writable dir (a dev checkout's data/) or in the
  // app resources; ReadPathForFile throws with a clear message when missing.
  QString const src = GetPlatform().ReadPathForFile(name.toStdString(), "wr").c_str();
  if (!CopyQtFile(src, JoinPathQt({destDir, name})))
    throw std::runtime_error(std::string("Cannot copy file ") + name.toStdString() + " to " + destDir.toStdString());
}

void CopyToWritableDir(QString const & name, QString const & srcDir)
{
  QString const writableDir = GetPlatform().WritableDir().c_str();
  if (!CopyQtFile(JoinPathQt({srcDir, name}), JoinPathQt({writableDir, name})))
    throw std::runtime_error(std::string("Cannot copy file ") + name.toStdString() + " from " + srcDir.toStdString());
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

QString GetExternalPath(QString const & name, QString const & relativePath)
{
  if (!relativePath.isEmpty())
  {
    // 1. Relative to the resources dir, which in a dev checkout is <repo>/data/ and in the
    // Designer package is <package>/data/, so "../tools/..." resolves into the repository or
    // the package.
    QString const resourceDir = GetPlatform().ResourcesDir().c_str();
    QString path = JoinPathQt({resourceDir, relativePath, name});
    if (QFileInfo::exists(path))
      return path;

    // 2. Same, relative to the writable dir, which on macOS is the data/ next to the .app rather
    // than its Resources. It is often reached through a <build>/data symlink, which a lexical
    // "data/.." would escape wrongly — canonicalize it first.
    QString const writableDir = QDir(QString::fromStdString(GetPlatform().WritableDir())).canonicalPath();
    path = JoinPathQt({writableDir, relativePath, name});
    if (QFileInfo::exists(path))
      return path;
  }

  // 3. Next to the app: <build>/ from a plain `cmake --build`, or the Designer package's root.
  // applicationDirPath() inside an .app is <dir>/<bundle>.app/Contents/MacOS, so walk out of it.
  QDir appDir(QCoreApplication::applicationDirPath());
  for (auto const & dirName : {QStringLiteral("MacOS"), QStringLiteral("Contents")})
    if (appDir.dirName() == dirName)
      appDir.cdUp();
  if (appDir.dirName().endsWith(QStringLiteral(".app")))
    appDir.cdUp();

  QString const path = JoinPathQt({appDir.absolutePath(), name});
  if (QFileInfo::exists(path))
    return path;

  throw std::runtime_error("Cannot find " + name.toStdString() +
                           "; build all desktop targets and run the app from the repository root");
}
