#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shader_def_for_tests.hpp"

#include "drape/shader.hpp"

#include "platform/platform.hpp"

#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include <QTemporaryFile>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QTextStream>

using namespace dp;

#if defined(OMIM_OS_MAC)

#define SHADERS_COMPILER "GLSLESCompiler_Series5.mac"
#define MALI_SHADERS_COMPILER "mali_compiler/malisc"
#define MALI_DIR "mali_compiler/"

std::string DebugPrint(QString const & s) { return s.toStdString(); }

void WriteShaderToFile(QTemporaryFile & file, string shader)
{
  QTextStream out(&file);
  out << QString::fromStdString(shader);
}

using PrepareProcessFn = std::function<void(QProcess & p)>;
using PrepareArgumentsFn = std::function<void(QStringList & args, QString const & fileName)>;
using SuccessComparator = std::function<bool(QString const & output)>;

void RunShaderTest(std::string const & shaderName, QString const & glslCompiler,
                   QString const & fileName, PrepareProcessFn const & procPrepare,
                   PrepareArgumentsFn const & argsPrepare,
                   SuccessComparator const & successComparator, QTextStream & errorLog)
{
  QProcess p;
  procPrepare(p);
  p.setProcessChannelMode(QProcess::MergedChannels);
  QStringList args;
  argsPrepare(args, fileName);
  p.start(glslCompiler, args, QIODevice::ReadOnly);

  TEST(p.waitForStarted(), ("GLSL compiler not started"));
  TEST(p.waitForFinished(), ("GLSL compiler not finished in time"));

  QString result = p.readAllStandardOutput();
  if (!successComparator(result))
  {
    errorLog << "\n" << QString("SHADER COMPILE ERROR\n");
    errorLog << QString(shaderName.c_str()) << "\n";
    errorLog << result.trimmed() << "\n";
  }
}

void ForEachShader(std::string const & defines,
                   vector<std::pair<std::string, std::string>> const & shaders,
                   QString const & glslCompiler, PrepareProcessFn const & procPrepare,
                   PrepareArgumentsFn const & argsPrepare,
                   SuccessComparator const & successComparator, QTextStream & errorLog)
{
  for (auto const & src : shaders)
  {
    QTemporaryFile srcFile;
    TEST(srcFile.open(), ("Temporary file can't be created!"));
    std::string fullSrc = defines + src.second;
    WriteShaderToFile(srcFile, fullSrc);
    RunShaderTest(src.first, glslCompiler, srcFile.fileName(), procPrepare, argsPrepare,
                  successComparator, errorLog);
  }
}

UNIT_TEST(CompileShaders_Test)
{
  Platform & platform = GetPlatform();
  std::string glslCompilerPath = platform.ResourcesDir() + "shaders_compiler/" SHADERS_COMPILER;
  if (!platform.IsFileExistsByFullPath(glslCompilerPath))
  {
    glslCompilerPath = platform.WritableDir() + "shaders_compiler/" SHADERS_COMPILER;
    TEST(platform.IsFileExistsByFullPath(glslCompilerPath), ("GLSL compiler not found"));
  }

  QString errorLog;
  QTextStream ss(&errorLog);

  QString compilerPath = QString::fromStdString(glslCompilerPath);
  QString shaderType = "-v";
  auto argsPrepareFn = [&shaderType](QStringList & args, QString const & fileName) {
    args << fileName << fileName + ".bin" << shaderType;
  };
  auto successComparator = [](QString const & output) { return output.indexOf("Success") != -1; };

  string defines = "";
  ForEachShader(defines, gpu::GetVertexEnum(), compilerPath, [](QProcess const &) {}, argsPrepareFn,
                successComparator, ss);
  shaderType = "-f";
  ForEachShader(defines, gpu::GetFragmentEnum(), compilerPath, [](QProcess const &) {},
                argsPrepareFn, successComparator, ss);

  TEST_EQUAL(errorLog.isEmpty(), true, ("PVR without defines :", errorLog));

  defines = "#define ENABLE_VTF\n";
  errorLog.clear();
  shaderType = "-v";
  ForEachShader(defines, gpu::GetVertexEnum(), compilerPath, [](QProcess const &) {}, argsPrepareFn,
                successComparator, ss);
  shaderType = "-f";
  ForEachShader(defines, gpu::GetFragmentEnum(), compilerPath, [](QProcess const &) {},
                argsPrepareFn, successComparator, ss);

  TEST_EQUAL(errorLog.isEmpty(), true, ("PVR with defines : ", defines, "\n", errorLog));

  defines = "#define SAMSUNG_GOOGLE_NEXUS\n";
  errorLog.clear();
  shaderType = "-v";
  ForEachShader(defines, gpu::GetVertexEnum(), compilerPath, [](QProcess const &) {}, argsPrepareFn,
                successComparator, ss);
  shaderType = "-f";
  ForEachShader(defines, gpu::GetFragmentEnum(), compilerPath, [](QProcess const &) {},
                argsPrepareFn, successComparator, ss);

  TEST_EQUAL(errorLog.isEmpty(), true, ("PVR with defines : ", defines, "\n", errorLog));
}

void TestMaliShaders(QString const & driver, QString const & hardware, QString const & release)
{
  Platform & platform = GetPlatform();
  std::string glslCompilerPath = platform.ResourcesDir() + "shaders_compiler/" MALI_SHADERS_COMPILER;
  TEST(platform.IsFileExistsByFullPath(glslCompilerPath), ("GLSL MALI compiler not found"));

  QString errorLog;
  QTextStream ss(&errorLog);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("MALICM_LOCATION",
             QString::fromStdString(platform.ResourcesDir() + "shaders_compiler/" MALI_DIR));
  auto procPrepare = [&env](QProcess & p) { p.setProcessEnvironment(env); };
  QString shaderType = "-v";
  auto argForming = [&](QStringList & args, QString const & fileName) {
    args << shaderType << "-V"
         << "-r" << release << "-c" << hardware << "-d" << driver << fileName;
  };

  auto succesComparator = [](QString const & output) {
    return output.indexOf("Compilation succeeded.") != -1;
  };

  std::string defines = "";
  QString const compilerPath = QString::fromStdString(glslCompilerPath);
  ForEachShader(defines, gpu::GetVertexEnum(), compilerPath, procPrepare, argForming,
                succesComparator, ss);
  shaderType = "-f";
  ForEachShader(defines, gpu::GetFragmentEnum(), compilerPath, procPrepare, argForming,
                succesComparator, ss);
  TEST(errorLog.isEmpty(), (shaderType, release, hardware, driver, defines, errorLog));

  // MALI GPUs do not support ENABLE_VTF. Do not test it here.
}

UNIT_TEST(MALI_CompileShaders_Test)
{
  typedef pair<QString, QString> TReleaseVersion;
  typedef vector<TReleaseVersion> TReleases;

  struct DriverSet
  {
    TReleases m_releases;
    QString m_driverName;
  };

  vector<DriverSet> models(3);
  models[0].m_driverName = "Mali-400_r4p0-00rel1";
  models[1].m_driverName = "Mali-T600_r4p0-00rel0";
  models[2].m_driverName = "Mali-T600_r4p1-00rel0";

  models[0].m_releases.push_back(make_pair("Mali-200", "r0p1"));
  models[0].m_releases.push_back(make_pair("Mali-200", "r0p2"));
  models[0].m_releases.push_back(make_pair("Mali-200", "r0p3"));
  models[0].m_releases.push_back(make_pair("Mali-200", "r0p4"));
  models[0].m_releases.push_back(make_pair("Mali-200", "r0p5"));
  models[0].m_releases.push_back(make_pair("Mali-200", "r0p6"));
  models[0].m_releases.push_back(make_pair("Mali-400", "r0p0"));
  models[0].m_releases.push_back(make_pair("Mali-400", "r0p1"));
  models[0].m_releases.push_back(make_pair("Mali-400", "r1p0"));
  models[0].m_releases.push_back(make_pair("Mali-400", "r1p1"));
  models[0].m_releases.push_back(make_pair("Mali-300", "r0p0"));
  models[0].m_releases.push_back(make_pair("Mali-450", "r0p0"));

  models[1].m_releases.push_back(make_pair("Mali-T600", "r0p0"));
  models[1].m_releases.push_back(make_pair("Mali-T600", "r0p0_15dev0"));
  models[1].m_releases.push_back(make_pair("Mali-T600", "r0p1"));
  models[1].m_releases.push_back(make_pair("Mali-T620", "r0p1"));
  models[1].m_releases.push_back(make_pair("Mali-T620", "r1p0"));
  models[1].m_releases.push_back(make_pair("Mali-T670", "r1p0"));

  models[2].m_releases.push_back(make_pair("Mali-T600", "r0p0"));
  models[2].m_releases.push_back(make_pair("Mali-T600", "r0p0_15dev0"));
  models[2].m_releases.push_back(make_pair("Mali-T600", "r0p1"));
  models[2].m_releases.push_back(make_pair("Mali-T620", "r0p1"));
  models[2].m_releases.push_back(make_pair("Mali-T620", "r1p0"));
  models[2].m_releases.push_back(make_pair("Mali-T620", "r1p1"));
  models[2].m_releases.push_back(make_pair("Mali-T720", "r0p0"));
  models[2].m_releases.push_back(make_pair("Mali-T720", "r1p0"));
  models[2].m_releases.push_back(make_pair("Mali-T760", "r0p0"));
  models[2].m_releases.push_back(make_pair("Mali-T760", "r0p1"));
  models[2].m_releases.push_back(make_pair("Mali-T760", "r0p1_50rel0"));
  models[2].m_releases.push_back(make_pair("Mali-T760", "r0p2"));
  models[2].m_releases.push_back(make_pair("Mali-T760", "r0p3"));
  models[2].m_releases.push_back(make_pair("Mali-T760", "r1p0"));

  for (auto const & set : models)
  {
    for (auto const & version : set.m_releases)
      TestMaliShaders(set.m_driverName, version.first, version.second);
  }
}
#endif
