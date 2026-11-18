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
#include <QtCore/QStandardPaths>

#include <exception>
#include <string>

QString ExecProcess(QString const & program, std::initializer_list<QString> args, QProcessEnvironment const * env,
                    bool mergeOutput)
{
  QStringList const qargs(args);

  QProcess p;
  if (mergeOutput)
    p.setProcessChannelMode(QProcess::MergedChannels);
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

QString GetScriptPath(QString const & name, QString const & relativeDir)
{
  auto const & platform = GetPlatform();
  for (auto const & dir : {platform.ResourcesDir(), platform.WritableDir()})
  {
    // Resolve data/ symlinks before walking up to ../tools in a checkout or package.
    auto const dataDir = QDir(QString::fromStdString(dir)).canonicalPath();
    if (dataDir.isEmpty())
      continue;
    auto const path = JoinPathQt({dataDir, relativeDir, name});
    if (QFileInfo(path).isFile())
      return path;
  }

  throw std::runtime_error("Cannot find " + name.toStdString() +
                           "; run the app from the repository root or use a Designer package");
}

QString GetHelperPath(QString const & name)
{
  // Packaged macOS helpers share Contents/MacOS with the app, so their Qt dependencies resolve
  // against the same bundled Frameworks directory. Development helpers live outside the bundle.
  QDir appDir(QCoreApplication::applicationDirPath());
  QStringList binaryDirs{appDir.absolutePath()};
  for (auto const & dirName : {QStringLiteral("MacOS"), QStringLiteral("Contents")})
    if (appDir.dirName() == dirName)
      appDir.cdUp();
  if (appDir.dirName().endsWith(QStringLiteral(".app")))
    appDir.cdUp();
  if (appDir.absolutePath() != binaryDirs.front())
    binaryDirs.push_back(appDir.absolutePath());

  // Qt adds the platform's executable suffix and checks executability.
  if (auto const path = QStandardPaths::findExecutable(name, binaryDirs); !path.isEmpty())
    return path;

  throw std::runtime_error("Cannot find " + name.toStdString() +
                           "; build all desktop targets and run the app from the repository root");
}
