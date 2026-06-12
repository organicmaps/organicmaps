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

void CopyFromDataDir(QString const & name, QString const & destDir)
{
  // The file may live in the writable dir (a dev checkout's data/) or in the
  // app resources; ReadPathForFile throws with a clear message when missing.
  QString const src = GetPlatform().ReadPathForFile(name.toStdString(), "wr").c_str();
  if (!CopyFile(src, JoinPathQt({destDir, name})))
    throw std::runtime_error(std::string("Cannot copy file ") + name.toStdString() + " to " + destDir.toStdString());
}

void CopyToWritableDir(QString const & name, QString const & srcDir)
{
  QString const writableDir = GetPlatform().WritableDir().c_str();
  if (!CopyFile(JoinPathQt({srcDir, name}), JoinPathQt({writableDir, name})))
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

QString GetExternalPath(QString const & name, QString const & primaryPath, QString const & secondaryPath)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();

  // 1. Bundled into the running .app's Resources via install/CPack.
  QString path = JoinPathQt({resourceDir, primaryPath, name});
  if (QFileInfo::exists(path))
    return path;
  if (!secondaryPath.isEmpty())
  {
    path = JoinPathQt({resourceDir, secondaryPath, name});
    if (QFileInfo::exists(path))
      return path;
  }

  // 2. Sibling in the build dir from a plain `cmake --build` (no install step).
  // applicationDirPath() inside an .app is <build>/<bundle>.app/Contents/MacOS;
  // walk out of the .app to <build> and look there too.
  QDir buildDir(QCoreApplication::applicationDirPath());
  for (auto const & dirName : {QStringLiteral("MacOS"), QStringLiteral("Contents")})
    if (buildDir.dirName() == dirName)
      buildDir.cdUp();
  if (buildDir.dirName().endsWith(QStringLiteral(".app")))
    buildDir.cdUp();

  if (!primaryPath.isEmpty())
  {
    path = JoinPathQt({buildDir.absolutePath(), primaryPath, name});
    if (QFileInfo::exists(path))
      return path;
  }
  // The helpers can also be raw binaries directly in <build>/ (cmake --build
  // produces e.g. <build>/skin_generator_tool, not a .app bundle).
  path = JoinPathQt({buildDir.absolutePath(), name});
  if (QFileInfo::exists(path))
    return path;

  return {};
}
