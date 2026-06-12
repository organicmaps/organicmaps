#pragma once

#include <QtCore/QString>

#include <initializer_list>
#include <string>
#include <utility>

class QProcessEnvironment;

// Returns stdout output of the program; throws std::runtime_error when the
// program cannot be started, crashes or returns a non-zero exit code.
// Arguments are passed to QProcess verbatim and must not be quoted manually.
QString ExecProcess(QString const & program, std::initializer_list<QString> args,
                    QProcessEnvironment const * env = nullptr);

bool CopyFile(QString const & oldFile, QString const & newFile);

// Copies a data file (resolved via the writable dir first, then app resources) into destDir.
void CopyFromDataDir(QString const & name, QString const & destDir);
// Copies srcDir/name into the writable dir, where readers look first ("wrf" scope).
void CopyToWritableDir(QString const & name, QString const & srcDir);

QString JoinPathQt(std::initializer_list<QString> folders);

QString GetExternalPath(QString const & name, QString const & primaryPath, QString const & secondaryPath);
