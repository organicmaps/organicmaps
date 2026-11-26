#pragma once

#include "std/target_os.hpp"

#include <QtCore/QString>

#include <initializer_list>

class QProcessEnvironment;

#ifdef OMIM_OS_WINDOWS
inline constexpr char const kPythonExecutable[] = "python";
#else
inline constexpr char const kPythonExecutable[] = "python3";
#endif

// Returns stdout output of the program; throws std::runtime_error when the
// program cannot be started, crashes or returns a non-zero exit code.
// Arguments are passed to QProcess verbatim and must not be quoted manually.
// With mergeOutput, returns both stdout and stderr (for tools that log reports to stderr).
QString ExecProcess(QString const & program, std::initializer_list<QString> args,
                    QProcessEnvironment const * env = nullptr, bool mergeOutput = false);

bool CopyQtFile(QString const & oldFile, QString const & newFile);

// Copies a data file (resolved via the writable dir first, then app resources) into destDir.
void CopyFromDataDir(QString const & name, QString const & destDir);
// Copies srcDir/name into the writable dir, where readers look first ("wrf" scope).
void CopyToWritableDir(QString const & name, QString const & srcDir);

QString JoinPathQt(std::initializer_list<QString> folders);

// Finds a Python script relative to the resources or writable directory; throws when missing.
QString GetScriptPath(QString const & name, QString const & relativeDir);
// Finds an executable next to the app or outside its macOS bundle, with the native suffix.
QString GetHelperPath(QString const & name);
