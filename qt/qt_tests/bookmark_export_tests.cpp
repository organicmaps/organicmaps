#include "testing/testing.hpp"

#include "qt/bookmark_export.hpp"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>

#include <filesystem>
#include <system_error>

namespace bookmark_export_tests
{
void WriteFile(QString const & path, QByteArray const & data)
{
  QFile file(path);
  TEST(file.open(QIODevice::WriteOnly), (file.errorString().toStdString()));
  TEST_EQUAL(file.write(data), data.size(), ());
}

QByteArray ReadFile(QString const & path)
{
  QFile file(path);
  TEST(file.open(QIODevice::ReadOnly), (file.errorString().toStdString()));
  return file.readAll();
}

UNIT_TEST(BookmarkExport_OverwriteExistingFile)
{
  QTemporaryDir dir;
  TEST(dir.isValid(), ());
  auto const source = dir.filePath("source.kmz");
  auto const destination = dir.filePath("destination.kmz");
  QByteArray const data(128 * 1024 + 17, 'x');
  WriteFile(source, data);
  WriteFile(destination, "existing export");

  TEST(qt::SaveExportedFile(source, destination), ());
  TEST(ReadFile(destination) == data, ());
  TEST(!QFile::exists(source), ());
}

UNIT_TEST(BookmarkExport_SameFileIsPreserved)
{
  QTemporaryDir dir;
  TEST(dir.isValid(), ());
  auto const source = dir.filePath(QString::fromUtf8("分類.kmz"));
  WriteFile(source, "export");

  TEST(qt::SaveExportedFile(source, source), ());
  TEST(ReadFile(source) == "export", ());

  auto const alias = dir.filePath(QString::fromUtf8("./分類.kmz"));
  TEST(qt::SaveExportedFile(source, alias), ());
  TEST(ReadFile(source) == "export", ());
}

UNIT_TEST(BookmarkExport_MissingSourcePreservesDestination)
{
  QTemporaryDir dir;
  TEST(dir.isValid(), ());
  auto const source = dir.filePath("missing.kmz");
  auto const destination = dir.filePath("destination.kmz");
  WriteFile(destination, "existing export");

  TEST(!qt::SaveExportedFile(source, destination), ());
  TEST(ReadFile(destination) == "existing export", ());
  TEST(!qt::SaveExportedFile(source, source), ());
}

UNIT_TEST(BookmarkExport_FileIdentityPreservesBothPaths)
{
  QTemporaryDir dir;
  TEST(dir.isValid(), ());
  auto const source = dir.filePath("source.kmz");
  auto const alias = dir.filePath("alias.kmz");
  WriteFile(source, "export");
  std::error_code error;
  std::filesystem::create_hard_link(QFileInfo(source).filesystemFilePath(), QFileInfo(alias).filesystemFilePath(),
                                    error);
  TEST(!error, (error.message()));

  TEST(qt::SaveExportedFile(source, alias), ());
  TEST(ReadFile(source) == "export", ());
  TEST(ReadFile(alias) == "export", ());
}

UNIT_TEST(BookmarkExport_SaveFailurePreservesSource)
{
  QTemporaryDir dir;
  TEST(dir.isValid(), ());
  auto const source = dir.filePath("source.kmz");
  WriteFile(source, "export");

  TEST(!qt::SaveExportedFile(source, dir.filePath("missing/destination.kmz")), ());
  TEST(ReadFile(source) == "export", ());
}
}  // namespace bookmark_export_tests
