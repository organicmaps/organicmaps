#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shader_def_for_tests.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include <QTemporaryFile>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QTextStream>

#if defined(OMIM_OS_MAC)

std::string const kCompilersDir = "shaders_compiler";
std::string const kMaliCompilerOpenGLES2Dir = "mali_compiler";
std::string const kMaliCompilerOpenGLES3Dir = "mali_compiler_es3";
std::string const kCompilerOpenGLES2 = "GLSLESCompiler_Series5.mac";
std::string const kCompilerMaliOpenGLES2 = kMaliCompilerOpenGLES2Dir + "/malisc";
std::string const kCompilerOpenGLES3 = "GLSLESCompiler_Series6.mac";
std::string const kCompilerMaliOpenGLES3 = kMaliCompilerOpenGLES3Dir + "/malisc";

std::string DebugPrint(QString const & s) { return s.toStdString(); }
std::string DebugPrint(dp::ApiVersion apiVersion)
{
  if (apiVersion == dp::OpenGLES2)
    return "OpenGLES2";
  else if (apiVersion == dp::OpenGLES3)
    return "OpenGLES3";
  return "Unknown";
}

namespace
{
void WriteShaderToFile(QTemporaryFile & file, std::string const & shader)
{
  QTextStream out(&file);
  out << QString::fromStdString(shader);
}

using PrepareProcessFn = std::function<void(QProcess & p)>;
using PrepareArgumentsFn = std::function<void(QStringList & args, QString const & fileName)>;
using SuccessChecker = std::function<bool(QString const & output)>;

void RunShaderTest(dp::ApiVersion apiVersion, std::string const & shaderName,
                   QString const & glslCompiler, QString const & fileName,
                   PrepareProcessFn const & procPrepare, PrepareArgumentsFn const & argsPrepare,
                   SuccessChecker const & successChecker, QTextStream & errorLog)
{
  QProcess p;
  procPrepare(p);
  p.setProcessChannelMode(QProcess::MergedChannels);
  QStringList args;
  argsPrepare(args, fileName);
  p.start(glslCompiler, args, QIODevice::ReadOnly);

  TEST(p.waitForStarted(), ("GLSL compiler not started", glslCompiler));
  TEST(p.waitForFinished(), ("GLSL compiler not finished in time", glslCompiler));

  QString result = p.readAllStandardOutput();
  if (!successChecker(result))
  {
    errorLog << "\n"
             << QString(DebugPrint(apiVersion).c_str()) << ": " << QString(shaderName.c_str())
             << QString(": SHADER COMPILE ERROR:\n");
    errorLog << result.trimmed() << "\n";
  }
}

void TestShaders(dp::ApiVersion apiVersion, std::string const & defines,
                 gpu::ShadersEnum const & shaders, QString const & glslCompiler,
                 PrepareProcessFn const & procPrepare, PrepareArgumentsFn const & argsPrepare,
                 SuccessChecker const & successChecker, QTextStream & errorLog)
{
  for (auto const & src : shaders)
  {
    QTemporaryFile srcFile;
    TEST(srcFile.open(), ("Temporary file can't be created!"));
    std::string fullSrc;
    if (src.second.find("#version") != std::string::npos)
    {
      auto pos = src.second.find('\n');
      ASSERT_NOT_EQUAL(pos, std::string::npos, ());
      fullSrc = src.second;
      fullSrc.insert(pos + 1, defines);
    }
    else
    {
      fullSrc = defines + src.second;
    }
    WriteShaderToFile(srcFile, fullSrc);
    RunShaderTest(apiVersion, src.first, glslCompiler, srcFile.fileName(), procPrepare, argsPrepare,
                  successChecker, errorLog);
  }
}

std::string GetCompilerPath(std::string const & compilerName)
{
  Platform & platform = GetPlatform();
  std::string compilerPath =
      my::JoinFoldersToPath({platform.ResourcesDir(), kCompilersDir}, compilerName);
  if (!platform.IsFileExistsByFullPath(compilerPath))
  {
    compilerPath = my::JoinFoldersToPath({platform.WritableDir(), kCompilersDir}, compilerName);
    TEST(platform.IsFileExistsByFullPath(compilerPath), ("GLSL compiler not found"));
  }
  return compilerPath;
}
}  // namespace

UNIT_TEST(CompileShaders_Test)
{
  struct CompilerData
  {
    dp::ApiVersion m_apiVersion;
    std::string m_compilerPath;
  };
  std::vector<CompilerData> const compilers = {
      {dp::ApiVersion::OpenGLES2, GetCompilerPath(kCompilerOpenGLES2)},
      {dp::ApiVersion::OpenGLES3, GetCompilerPath(kCompilerOpenGLES3)},
  };

  auto successChecker = [](QString const & output) { return output.indexOf("Success") != -1; };

  for (auto const & compiler : compilers)
  {
    QString errorLog;
    QTextStream ss(&errorLog);

    QString compilerPath = QString::fromStdString(compiler.m_compilerPath);
    QString shaderType = "-v";
    auto argsPrepareFn = [&shaderType](QStringList & args, QString const & fileName) {
      args << fileName << fileName + ".bin" << shaderType;
    };

    string defines = "";
    TestShaders(compiler.m_apiVersion, defines, gpu::GetVertexShaders(compiler.m_apiVersion),
                compilerPath, [](QProcess const &) {}, argsPrepareFn, successChecker, ss);
    shaderType = "-f";
    TestShaders(compiler.m_apiVersion, defines, gpu::GetFragmentShaders(compiler.m_apiVersion),
                compilerPath, [](QProcess const &) {}, argsPrepareFn, successChecker, ss);

    TEST_EQUAL(errorLog.isEmpty(), true, ("PVR without defines :", errorLog));

    defines = "#define ENABLE_VTF\n";
    errorLog.clear();
    shaderType = "-v";
    TestShaders(compiler.m_apiVersion, defines, gpu::GetVertexShaders(compiler.m_apiVersion),
                compilerPath, [](QProcess const &) {}, argsPrepareFn, successChecker, ss);
    shaderType = "-f";
    TestShaders(compiler.m_apiVersion, defines, gpu::GetFragmentShaders(compiler.m_apiVersion),
                compilerPath, [](QProcess const &) {}, argsPrepareFn, successChecker, ss);

    TEST_EQUAL(errorLog.isEmpty(), true, ("PVR with defines : ", defines, "\n", errorLog));

    defines = "#define SAMSUNG_GOOGLE_NEXUS\n";
    errorLog.clear();
    shaderType = "-v";
    TestShaders(compiler.m_apiVersion, defines, gpu::GetVertexShaders(compiler.m_apiVersion),
                compilerPath, [](QProcess const &) {}, argsPrepareFn, successChecker, ss);
    shaderType = "-f";
    TestShaders(compiler.m_apiVersion, defines, gpu::GetFragmentShaders(compiler.m_apiVersion),
                compilerPath, [](QProcess const &) {}, argsPrepareFn, successChecker, ss);

    TEST_EQUAL(errorLog.isEmpty(), true, ("PVR with defines : ", defines, "\n", errorLog));
  }
}

UNIT_TEST(MALI_CompileShaders_Test)
{
  using ReleaseVersion = std::pair<QString, QString>;
  using Releases = std::vector<ReleaseVersion>;

  struct DriverSet
  {
    QString m_driverName;
    Releases m_releases;
  };

  struct CompilerData
  {
    dp::ApiVersion m_apiVersion;
    std::string m_compilerPath;
    std::string m_compilerAdditionalPath;
    std::vector<DriverSet> m_driverSets;
  };

  std::vector<CompilerData> const compilers = {
      {dp::ApiVersion::OpenGLES2,
       GetCompilerPath(kCompilerMaliOpenGLES2),
       GetCompilerPath(kMaliCompilerOpenGLES2Dir),
       {{"Mali-400_r4p0-00rel1",
         {std::make_pair("Mali-200", "r0p1"), std::make_pair("Mali-200", "r0p2"),
          std::make_pair("Mali-200", "r0p3"), std::make_pair("Mali-200", "r0p4"),
          std::make_pair("Mali-200", "r0p5"), std::make_pair("Mali-200", "r0p6"),
          std::make_pair("Mali-400", "r0p0"), std::make_pair("Mali-400", "r0p1"),
          std::make_pair("Mali-400", "r1p0"), std::make_pair("Mali-400", "r1p1"),
          std::make_pair("Mali-300", "r0p0"), std::make_pair("Mali-450", "r0p0")}},
        {"Mali-T600_r4p0-00rel0",
         {std::make_pair("Mali-T600", "r0p0"), std::make_pair("Mali-T600", "r0p0_15dev0"),
          std::make_pair("Mali-T600", "r0p1"), std::make_pair("Mali-T620", "r0p1"),
          std::make_pair("Mali-T620", "r1p0"), std::make_pair("Mali-T670", "r1p0")}},
        {"Mali-T600_r4p1-00rel0",
         {std::make_pair("Mali-T600", "r0p0"), std::make_pair("Mali-T600", "r0p0_15dev0"),
          std::make_pair("Mali-T600", "r0p1"), std::make_pair("Mali-T620", "r0p1"),
          std::make_pair("Mali-T620", "r1p0"), std::make_pair("Mali-T620", "r1p1"),
          std::make_pair("Mali-T720", "r0p0"), std::make_pair("Mali-T720", "r1p0"),
          std::make_pair("Mali-T760", "r0p0"), std::make_pair("Mali-T760", "r0p1"),
          std::make_pair("Mali-T760", "r0p1_50rel0"), std::make_pair("Mali-T760", "r0p2"),
          std::make_pair("Mali-T760", "r0p3"), std::make_pair("Mali-T760", "r1p0")}}}},
      {dp::ApiVersion::OpenGLES3,
       GetCompilerPath(kCompilerMaliOpenGLES3),
       GetCompilerPath(kMaliCompilerOpenGLES3Dir),
       {{"Mali-T600_r5p0-00rel0",
         {std::make_pair("Mali-T600", "r0p0"), std::make_pair("Mali-T600", "r0p0_15dev0"),
          std::make_pair("Mali-T600", "r0p1"), std::make_pair("Mali-T620", "r0p1"),
          std::make_pair("Mali-T620", "r1p0"), std::make_pair("Mali-T620", "r1p1"),
          std::make_pair("Mali-T760", "r0p0"), std::make_pair("Mali-T760", "r0p1"),
          std::make_pair("Mali-T760", "r0p1_50rel0"), std::make_pair("Mali-T760", "r0p2"),
          std::make_pair("Mali-T760", "r0p3"), std::make_pair("Mali-T760", "r1p0")}},
        {"Mali-T600_r5p1-00rel0",
         {std::make_pair("Mali-T600", "r0p0"), std::make_pair("Mali-T600", "r0p0_15dev0"),
          std::make_pair("Mali-T600", "r0p1"), std::make_pair("Mali-T620", "r0p1"),
          std::make_pair("Mali-T620", "r1p0"), std::make_pair("Mali-T620", "r1p1"),
          std::make_pair("Mali-T760", "r0p0"), std::make_pair("Mali-T760", "r0p1"),
          std::make_pair("Mali-T760", "r0p1_50rel0"), std::make_pair("Mali-T760", "r0p2"),
          std::make_pair("Mali-T760", "r0p3"), std::make_pair("Mali-T760", "r1p0")}},
        {"Mali-T600_r6p0-00rel0",
         {std::make_pair("Mali-T600", "r0p0"), std::make_pair("Mali-T600", "r0p0_15dev0"),
          std::make_pair("Mali-T600", "r0p1"), std::make_pair("Mali-T620", "r0p1"),
          std::make_pair("Mali-T620", "r1p0"), std::make_pair("Mali-T620", "r1p1"),
          std::make_pair("Mali-T760", "r0p0"), std::make_pair("Mali-T760", "r0p1"),
          std::make_pair("Mali-T760", "r0p1_50rel0"), std::make_pair("Mali-T760", "r0p2"),
          std::make_pair("Mali-T760", "r0p3"), std::make_pair("Mali-T760", "r1p0")}}}},
  };

  auto successChecker = [](QString const & output) {
    return output.indexOf("Compilation succeeded.") != -1;
  };

  for (auto const & compiler : compilers)
  {
    for (auto const & set : compiler.m_driverSets)
    {
      for (auto const & version : set.m_releases)
      {
        QString errorLog;
        QTextStream ss(&errorLog);

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("MALICM_LOCATION", QString::fromStdString(compiler.m_compilerAdditionalPath));
        auto procPrepare = [&env](QProcess & p) { p.setProcessEnvironment(env); };
        QString shaderType = "-v";
        auto argForming = [&](QStringList & args, QString const & fileName) {
          args << shaderType << "-V"
               << "-r" << version.second << "-c" << version.first << "-d" << set.m_driverName
               << fileName;
        };
        std::string defines = "";
        QString const compilerPath = QString::fromStdString(compiler.m_compilerPath);
        TestShaders(compiler.m_apiVersion, defines, gpu::GetVertexShaders(compiler.m_apiVersion),
                    compilerPath, procPrepare, argForming, successChecker, ss);
        shaderType = "-f";
        TestShaders(compiler.m_apiVersion, defines, gpu::GetFragmentShaders(compiler.m_apiVersion),
                    compilerPath, procPrepare, argForming, successChecker, ss);
        TEST(errorLog.isEmpty(),
             (shaderType, version.second, version.first, set.m_driverName, defines, errorLog));

        // MALI GPUs do not support ENABLE_VTF. Do not test it here.
        // SAMSUNG_GOOGLE_NEXUS doesn't use Mali GPU. Do not test it here.
      }
    }
  }
}
#endif
