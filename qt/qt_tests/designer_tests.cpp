#include "testing/testing.hpp"

#include "qt/build_style/build_common.h"
#include "qt/build_style/build_style.h"

#include "tools/skin_generator/generator.hpp"

#include "map/framework.hpp"
#include "search/search_params.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/files_container.hpp"

#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <stdexcept>

namespace designer_tests
{
namespace
{
void EnsureApp()
{
  if (!QCoreApplication::instance())
  {
    static int argc = 0;
    static QApplication app(argc, nullptr);
  }
}

void Write(QString const & path, QByteArray const & data)
{
  TEST(QDir().mkpath(QFileInfo(path).absolutePath()), (path.toStdString()));
  QFile file(path);
  TEST(file.open(QIODevice::WriteOnly), (path.toStdString()));
  TEST_EQUAL(file.write(data), data.size(), (path.toStdString()));
}

bool Throws(auto && action)
{
  try
  {
    action();
    return false;
  }
  catch (std::runtime_error const &)
  {
    return true;
  }
}

struct StyleFixture
{
  QTemporaryDir m_dir;
  QString m_mapcss;
  QString m_output;
  build_style::StyleInfo m_info;

  explicit StyleFixture(bool symbols = false)
  {
    m_mapcss = m_dir.path() + "/data/styles/default/light/style.mapcss";
    m_output = QFileInfo(m_mapcss).absolutePath() + "/out";
    Write(m_mapcss, "style source");
    TEST(build_style::TryParseStyleInfo(m_mapcss, m_info), ());
    Write(m_output + "/drules_default.bin", "drawing rules");
    if (symbols)
      TEST(QDir().mkpath(QFileInfo(m_mapcss).absolutePath() + "/symbols"), ());
  }
};
}  // namespace

UNIT_TEST(Designer_ProcessOutputPreservesReportsAndFailures)
{
  EnsureApp();
  QString const script = "import sys; print('stdout report'); print('stderr report', file=sys.stderr)";
  auto const stdoutOnly = ExecProcess(kPythonExecutable, {"-c", script});
  TEST(stdoutOnly.contains("stdout report"), ());
  TEST(!stdoutOnly.contains("stderr report"), ());
  auto const combined = ExecProcess(kPythonExecutable, {"-c", script}, nullptr, true /* mergeOutput */);
  TEST(combined.contains("stdout report"), ());
  TEST(combined.contains("stderr report"), ());
  bool failed = false;
  try
  {
    ExecProcess(kPythonExecutable, {"-c", script + "; sys.exit(7)"}, nullptr, true /* mergeOutput */);
  }
  catch (std::runtime_error const & error)
  {
    failed = true;
    std::string const report = error.what();
    TEST(report.find("stdout report") != std::string::npos, (report));
    TEST(report.find("stderr report") != std::string::npos, (report));
    TEST(report.find("Returned 7") != std::string::npos, (report));
  }
  TEST(failed, ());
}

UNIT_TEST(Designer_ResolvesNativeExecutableName)
{
  EnsureApp();
  QString path = QCoreApplication::applicationDirPath() + "/designer-helper-XXXXXX";
#ifdef OMIM_OS_WINDOWS
  path += ".exe";
#endif
  QTemporaryFile helper(path);
  TEST(helper.open(), ());
  TEST(helper.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner), ());
  auto const file = QFileInfo(helper);
#ifdef OMIM_OS_WINDOWS
  auto const name = file.completeBaseName();
#else
  auto const name = file.fileName();
#endif
  TEST_EQUAL(QFileInfo(GetHelperPath(name)).canonicalFilePath().toStdString(), file.canonicalFilePath().toStdString(),
             ());
}

UNIT_TEST(Designer_EditableWorldFollowsBundledVersion)
{
  QTemporaryDir dir;
  TEST(dir.isValid(), ());
  auto const resources = dir.path() + "/bundle";
  auto const writable = dir.path() + "/data";
  TEST(QDir().mkpath(resources), ());
  TEST(QDir().mkpath(writable), ());
  auto & platform = GetPlatform();
  auto const oldResources = platform.ResourcesDir();
  auto const oldWritable = platform.WritableDir();
  SCOPE_GUARD(restore, [&]
  {
    platform.SetResourceDir(oldResources);
    platform.SetWritableDirForTests(oldWritable);
  });
  platform.SetResourceDir(resources.toStdString());
  platform.SetWritableDirForTests(writable.toStdString());

  auto const writeWorld = [](QString const & path, uint64_t timestamp)
  {
    FilesContainerW container(path.toStdString());
    auto writer = container.GetWriter(VERSION_FILE_TAG);
    version::WriteVersion(*writer, timestamp);
  };
  uint64_t constexpr kOldTimestamp = 1609459200;  // 2021-01-01 UTC.
  uint64_t constexpr kNewTimestamp = 1640995200;  // 2022-01-01 UTC.
  writeWorld(resources + "/World.mwm", kOldTimestamp);
  writeWorld(resources + "/WorldCoasts.mwm", kOldTimestamp);
  writeWorld(writable + "/World.mwm", kOldTimestamp);  // A legacy root copy must not shadow newer resources.

  build_style::PrepareEditableWorld();
  auto const editable = writable + "/210101/World.mwm";
  TEST(QFileInfo::exists(editable), ());
  TEST(!QFileInfo::exists(writable + "/WorldCoasts.mwm"), ());
  {
    FilesContainerW container(editable.toStdString(), FileWriter::OP_WRITE_EXISTING);
    auto writer = container.GetWriter("edited");
    writer->Write("index", 5);
  }
  build_style::PrepareEditableWorld();
  TEST(FilesContainerR(editable.toStdString()).IsExist("edited"), ());

  auto const findWorld = [](int64_t latestVersion)
  {
    std::vector<platform::LocalCountryFile> maps;
    platform::FindAllLocalMapsAndCleanup(latestVersion, maps);
    std::sort(maps.begin(), maps.end(), [](auto const & a, auto const & b) { return a.GetVersion() > b.GetVersion(); });
    for (auto const & map : maps)
      if (map.GetCountryName() == "World")
        return map;
    TEST(false, ("World not registered"));
    return platform::LocalCountryFile();
  };
  auto world = findWorld(210101);
  TEST(!world.IsInBundle(), ());
  TEST(FilesContainerR(platform::GetCountryReader(world, MapFileType::Map)).IsExist("edited"), ());

  writeWorld(resources + "/World.mwm", kNewTimestamp);
  world = findWorld(220101);
  TEST(world.IsInBundle(), ());
  TEST_EQUAL(world.GetVersion(), 220101, ());
  TEST_EQUAL(version::ReadVersionDate(ModelReaderPtr(platform::GetCountryReader(world, MapFileType::Map))), 220101, ());
  build_style::PrepareEditableWorld();
  TEST(QFileInfo::exists(writable + "/220101/World.mwm"), ());
  world = findWorld(220101);
  TEST(!world.IsInBundle(), ());
  TEST_EQUAL(world.GetVersion(), 220101, ());
}

#ifndef OMIM_OS_WINDOWS
UNIT_TEST(Designer_ResolvesScriptsThroughDataSymlinks)
{
  QTemporaryDir dir;
  auto const data = dir.path() + "/repository/data";
  auto const alias = dir.path() + "/build/data";
  TEST(QDir().mkpath(data), ());
  TEST(QDir().mkpath(QFileInfo(alias).absolutePath()), ());
  TEST(QFile::link(data, alias), ());
  auto const script = dir.path() + "/repository/tools/python/script with spaces.py";
  Write(script, "script");
  auto & platform = GetPlatform();
  auto const resources = platform.ResourcesDir();
  auto const writable = platform.WritableDir();
  SCOPE_GUARD(restore, [&]
  {
    platform.SetResourceDir(resources);
    platform.SetWritableDirForTests(writable);
  });
  platform.SetResourceDir(alias.toStdString());
  platform.SetWritableDirForTests(alias.toStdString());
  TEST_EQUAL(GetScriptPath("script with spaces.py", "../tools/python").toStdString(),
             QFileInfo(script).canonicalFilePath().toStdString(), ());
}
#endif

UNIT_TEST(Designer_PhoneExportRejectsOverlappingSourcesBeforeConfirmation)
{
  StyleFixture fixture;
  bool asked = false;
  TEST(Throws(
           [&]
  {
    build_style::ExportPhonePackage(fixture.m_mapcss, fixture.m_info, fixture.m_dir.path() + "/data",
                                    [&](QString const &)
    {
      asked = true;
      return true;
    });
  }),
       ());
  TEST(!asked, ());
  TEST(QFileInfo::exists(fixture.m_mapcss), ());
  TEST(QFileInfo::exists(fixture.m_output + "/drules_default.bin"), ());
}

UNIT_TEST(Designer_MissingAtlasPreservesExistingExport)
{
  StyleFixture fixture(true);
  auto const target = fixture.m_dir.path() + "/export";
  Write(target + "/styles/keep", "existing package");
  bool asked = false;
  TEST(Throws(
           [&]
  {
    build_style::ExportPhonePackage(fixture.m_mapcss, fixture.m_info, target, [&](QString const &)
    {
      asked = true;
      return true;
    });
  }),
       ());
  TEST(!asked, ());
  TEST(QFileInfo::exists(target + "/styles/keep"), ());
}

UNIT_TEST(Designer_DeclinedOverwritePreservesExistingExport)
{
  StyleFixture fixture;
  auto const target = fixture.m_dir.path() + "/export";
  Write(target + "/styles/keep", "existing package");
  auto const result =
      build_style::ExportPhonePackage(fixture.m_mapcss, fixture.m_info, target, [](QString const &) { return false; });
  TEST(result.isEmpty(), ());
  TEST(QFileInfo::exists(target + "/styles/keep"), ());
}

UNIT_TEST(Designer_ExportsCompleteDefaultAtlas)
{
  StyleFixture fixture(true);
  auto const target = fixture.m_dir.path() + "/export";
  Write(target + "/styles/keep", "existing package");
  for (char const * dpi : {"mdpi", "hdpi", "xhdpi", "6plus", "xxhdpi", "xxxhdpi"})
    for (char const * leaf : {"symbols.png", "symbols.xml"})
      Write(fixture.m_output + "/symbols/" + dpi + "/light/" + leaf, "atlas");
  auto const result =
      build_style::ExportPhonePackage(fixture.m_mapcss, fixture.m_info, target, [](QString const &) { return true; });
  TEST_EQUAL(result.toStdString(), QDir(target + "/styles").canonicalPath().toStdString(), ());
  TEST(!QFileInfo::exists(result + "/keep"), ());
  TEST(QFileInfo::exists(result + "/drules_default.bin"), ());
  for (char const * dpi : {"mdpi", "hdpi", "xhdpi", "6plus", "xxhdpi", "xxxhdpi"})
    for (char const * leaf : {"symbols.png", "symbols.xml"})
      TEST(QFileInfo::exists(result + "/symbols/" + dpi + "/light/" + leaf), ());
  TEST(QFileInfo::exists(fixture.m_mapcss), ());
}

UNIT_TEST(Designer_SkinRejectsMalformedSvgAndOversizedInitialPage)
{
  EnsureApp();
  QTemporaryDir dir;
  auto const sources = dir.path() + "/symbols";
  auto const output = dir.path() + "/out";
  TEST(QDir().mkpath(output), ());
  Write(sources + "/good.svg",
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"18\" height=\"18\">"
        "<rect width=\"18\" height=\"18\" fill=\"red\"/></svg>");
  Write(sources + "/bad.svg", "<svg malformed");
  TEST(Throws([&] { tools::BuildSkin(sources, 18, 4096, output); }), ());
  TEST(QFile::remove(sources + "/bad.svg"), ());
  TEST(Throws([&] { tools::BuildSkin(sources, 18, 16, output); }), ());
  tools::BuildSkin(sources, 18, 4096, output);
  TEST(QFileInfo::exists(output + "/symbols.xml"), ());
}

UNIT_TEST(Designer_FixedStyleSurvivesDebugSearchCommands)
{
  FrameworkParams params;
  params.m_fixedMapStyle = MapStyleOutdoorsDark;
  Framework framework(params, false);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleOutdoorsDark, ());
  search::SearchParams query;
  query.m_query = "?light";
  TEST(framework.ParseSearchQueryCommand(query), ());
  TEST_EQUAL(framework.GetMapStyle(), MapStyleOutdoorsDark, ());
  framework.MarkMapStyle(MapStyleVehicleLight);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleOutdoorsDark, ());
}
}  // namespace designer_tests
