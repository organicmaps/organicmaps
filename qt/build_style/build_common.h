#pragma once

#include <QtCore/QString>

#include <initializer_list>
#include <string>
#include <utility>

class QProcessEnvironment;

// Returns stdout output of the program, throws std::runtime_error in case of non-zero exit code.
// Quotes all arguments to avoid issues with space-containing paths.
QString ExecProcess(QString const & program, std::initializer_list<QString> args, QProcessEnvironment const * env = nullptr);

bool CopyFile(QString const & oldFile, QString const & newFile);

void CopyFromResources(QString const & name, QString const & output);
void CopyToResources(QString const & name, QString const & input, QString const & newName = "");

QString JoinPathQt(std::initializer_list<QString> folders);

QString GetExternalPath(QString const & name, QString const & primaryPath,
                        QString const & secondaryPath);
QString GetProtobufEggPath();
