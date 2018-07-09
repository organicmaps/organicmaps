#pragma once

#include <QtCore/QString>
#include <QtCore/QProcessEnvironment>

#include <initializer_list>
#include <string>
#include <utility>

inline std::string to_string(const QString & qs) { return qs.toUtf8().constData(); }

std::pair<int, QString> ExecProcess(QString const & cmd, QProcessEnvironment const * env = nullptr);

bool CopyFile(QString const & oldFile, QString const & newFile);

void CopyFromResources(QString const & name, QString const & output);
void CopyToResources(QString const & name, QString const & input, QString const & newName = "");

QString JoinFoldersToPath(std::initializer_list<QString> const & folders);

QString GetExternalPath(QString const & name, QString const & primaryPath,
                        QString const & secondaryPath);
QString GetProtobufEggPath();
