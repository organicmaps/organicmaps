#include "testing/testing.hpp"

#include "shaders/gl_shaders.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "base/logging.hpp"
#include "base/thread_pool_delayed.hpp"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextStream>

#include <atomic>
#include <chrono>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

std::string const kCompilersDir = "shaders_compiler";

#if defined(OMIM_OS_MAC)
std::string const kMaliCompilerOpenGLES3Dir = "macos/mali_compiler_es3";
std::string const kCompilerMaliOpenGLES3 = kMaliCompilerOpenGLES3Dir + "/malisc";
std::string const kCompilerOpenGLES = "macos/glslangValidator";
#elif defined(OMIM_OS_LINUX)
std::string const kMaliCompilerOpenGLES3Dir = "linux/mali_compiler_es3";
std::string const kCompilerMaliOpenGLES3 = kMaliCompilerOpenGLES3Dir + "/malisc";
std::string const kCompilerOpenGLES = "linux/glslangValidator";
#endif

std::string DebugPrint(QString const & s)
{
  return s.toStdString();
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

std::map<std::string, std::string> GetVertexShaders(dp::ApiVersion apiVersion)
{
  std::map<std::string, std::string> shaders;
  for (size_t i = 0; i < static_cast<size_t>(gpu::Program::ProgramsCount); ++i)
  {
    auto const programInfo = gpu::GetProgramInfo(apiVersion, static_cast<gpu::Program>(i));
    shaders[programInfo.m_vertexShaderName] = programInfo.m_vertexShaderSource;
  }
  return shaders;
}

std::map<std::string, std::string> GetFragmentShaders(dp::ApiVersion apiVersion)
{
  std::map<std::string, std::string> shaders;
  for (size_t i = 0; i < static_cast<size_t>(gpu::Program::ProgramsCount); ++i)
  {
    auto const programInfo = gpu::GetProgramInfo(apiVersion, static_cast<gpu::Program>(i));
    shaders[programInfo.m_fragmentShaderName] = programInfo.m_fragmentShaderSource;
  }
  return shaders;
}

void RunShaderTest(dp::ApiVersion apiVersion, std::string const & shaderName, QString const & glslCompiler,
                   QString const & fileName, PrepareProcessFn const & procPrepare,
                   PrepareArgumentsFn const & argsPrepare, SuccessChecker const & successChecker,
                   QTextStream & errorLog)
{
  QProcess p;
  procPrepare(p);
  p.setProcessChannelMode(QProcess::MergedChannels);
  QStringList args;
  argsPrepare(args, fileName);
  p.start(glslCompiler, args, QIODevice::ReadOnly);

  TEST(p.waitForStarted(), ("GLSL compiler not started", glslCompiler));

  int32_t kFinishTimeoutInMs = 60000;
  TEST(p.waitForFinished(kFinishTimeoutInMs), ("GLSL compiler not finished in time", glslCompiler));

  QString result = p.readAllStandardOutput();
  if (!successChecker(result))
  {
    errorLog << "\n"
             << QString(DebugPrint(apiVersion).c_str()) << ": " << QString(shaderName.c_str())
             << QString(": SHADER COMPILE ERROR:\n");
    errorLog << result.trimmed() << "\n";
  }
}

void TestShaders(dp::ApiVersion apiVersion, std::string const & defines, QString const & ext,
                 std::map<std::string, std::string> const & shaders, QString const & glslCompiler,
                 PrepareProcessFn const & procPrepare, PrepareArgumentsFn const & argsPrepare,
                 SuccessChecker const & successChecker, QTextStream & errorLog)
{
  for (auto const & src : shaders)
  {
    // From QTemporaryFile documentation (https://doc.qt.io/qt-5/qtemporaryfile.html):
    // "Specified filenames can contain the following template XXXXXX (six upper case "X" characters),
    // which will be replaced by the auto-generated portion of the filename."
    QTemporaryFile srcFile(QString("XXXXXX") + ext);
    TEST(srcFile.open(), ("Temporary file can't be created!"));
    std::string fullSrc;
    if (apiVersion == dp::ApiVersion::OpenGLES3)
    {
      // Use desktop shader version for Linux
#ifdef OMIM_OS_LINUX
      if (glslCompiler.contains(kCompilerOpenGLES.c_str()))
        fullSrc = std::string(gpu::GL3_SHADER_VERSION) + defines + src.second;
      else
#endif
        fullSrc = std::string(gpu::GLES3_SHADER_VERSION) + defines + src.second;
    }
    else
    {
      fullSrc = defines + src.second;
    }
    WriteShaderToFile(srcFile, fullSrc);
    RunShaderTest(apiVersion, src.first, glslCompiler, srcFile.fileName(), procPrepare, argsPrepare, successChecker,
                  errorLog);
  }
}

std::string GetCompilerPath(std::string const & compilerName)
{
  Platform & platform = GetPlatform();
  std::string compilerPath = base::JoinPath(kCompilersDir, compilerName);
  TEST(platform.IsFileExistsByFullPath(compilerPath), (kCompilersDir, "should present in executable dir"));
  return compilerPath;
}
}  // namespace

struct CompilerData
{
  dp::ApiVersion m_apiVersion;
  std::string m_compilerPath;
};

void CompileShaders(CompilerData const & compiler, std::string const & additionalDefines = {})
{
  auto successChecker = [](QString const & output) { return output.isEmpty(); };

  QString errorLog;
  QTextStream ss(&errorLog);

  QString compilerPath = QString::fromStdString(compiler.m_compilerPath);
  auto argsPrepareFn = [](QStringList & args, QString const & fileName) { args << fileName; };

  TestShaders(compiler.m_apiVersion, additionalDefines, ".vert", GetVertexShaders(compiler.m_apiVersion), compilerPath,
              [](QProcess const &) {}, argsPrepareFn, successChecker, ss);
  TestShaders(compiler.m_apiVersion, additionalDefines, ".frag", GetFragmentShaders(compiler.m_apiVersion),
              compilerPath, [](QProcess const &) {}, argsPrepareFn, successChecker, ss);

  TEST_EQUAL(errorLog.isEmpty(), true, ("Defines:", additionalDefines, "\n", errorLog));
}

UNIT_TEST(MobileCompileShaders_Test)
{
  base::DelayedThreadPool workerThread(6 /* threadsCount */);

  workerThread.Push([] { CompileShaders({dp::ApiVersion::OpenGLES3, GetCompilerPath(kCompilerOpenGLES)}); });

  workerThread.Push([]
  { CompileShaders({dp::ApiVersion::OpenGLES3, GetCompilerPath(kCompilerOpenGLES)}, "#define ENABLE_VTF\n"); });

  workerThread.Shutdown(base::DelayedThreadPool::Exit::ExecPending);
}

struct MaliReleaseVersion
{
  QString m_series;
  QString m_version;
  bool m_availableForMacOS;
};

using MaliReleases = std::vector<MaliReleaseVersion>;

struct MaliDriverSet
{
  QString m_driverName;
  MaliReleases m_releases;
};

struct MaliCompilerData
{
  dp::ApiVersion m_apiVersion;
  std::string m_compilerPath;
  std::string m_compilerAdditionalPath;
  std::vector<MaliDriverSet> m_driverSets;
};

void MaliCompileShaders(MaliCompilerData const & compiler, MaliDriverSet const & driverSet,
                        MaliReleaseVersion const & version)
{
  auto successChecker = [](QString const & output) { return output.indexOf("Compilation succeeded.") != -1; };

  QString errorLog;
  QTextStream ss(&errorLog);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("MALICM_LOCATION", QString::fromStdString(compiler.m_compilerAdditionalPath));
  auto procPrepare = [&env](QProcess & p) { p.setProcessEnvironment(env); };
  QString shaderType = "-v";
  auto argForming = [&](QStringList & args, QString const & fileName)
  {
    args << shaderType << "-r" << version.m_version << "-c" << version.m_series << "-d" << driverSet.m_driverName
         << fileName;
  };
  QString const compilerPath = QString::fromStdString(compiler.m_compilerPath);
  TestShaders(compiler.m_apiVersion, "", {}, GetVertexShaders(compiler.m_apiVersion), compilerPath, procPrepare,
              argForming, successChecker, ss);
  shaderType = "-f";
  TestShaders(compiler.m_apiVersion, "", {}, GetFragmentShaders(compiler.m_apiVersion), compilerPath, procPrepare,
              argForming, successChecker, ss);
  TEST(errorLog.isEmpty(), (shaderType, version.m_series, version.m_version, driverSet.m_driverName, "", errorLog));

  // MALI GPUs do not support ENABLE_VTF. Do not test it here.
}

UNIT_TEST(MALI_MobileCompileShaders_Test)
{
#if defined(OMIM_OS_MAC)
  std::vector<MaliDriverSet> const driversES2old = {{"Mali-400_r4p0-00rel1",
                                                     {{"Mali-200", "r0p1", true},
                                                      {"Mali-200", "r0p2", true},
                                                      {"Mali-200", "r0p3", true},
                                                      {"Mali-200", "r0p4", true},
                                                      {"Mali-200", "r0p5", true},
                                                      {"Mali-200", "r0p6", true},
                                                      {"Mali-400", "r0p0", true},
                                                      {"Mali-400", "r0p1", true},
                                                      {"Mali-400", "r1p0", true},
                                                      {"Mali-400", "r1p1", true},
                                                      {"Mali-300", "r0p0", true},
                                                      {"Mali-450", "r0p0", true}}},
                                                    {"Mali-T600_r4p0-00rel0",
                                                     {{"Mali-T600", "r0p0", true},
                                                      {"Mali-T600", "r0p0_15dev0", true},
                                                      {"Mali-T600", "r0p1", true},
                                                      {"Mali-T620", "r0p1", true},
                                                      {"Mali-T620", "r1p0", true},
                                                      {"Mali-T670", "r1p0", true}}},
                                                    {"Mali-T600_r4p1-00rel0",
                                                     {{"Mali-T600", "r0p0", true},
                                                      {"Mali-T600", "r0p0_15dev0", true},
                                                      {"Mali-T600", "r0p1", true},
                                                      {"Mali-T620", "r0p1", true},
                                                      {"Mali-T620", "r1p0", true},
                                                      {"Mali-T620", "r1p1", true},
                                                      {"Mali-T720", "r0p0", true},
                                                      {"Mali-T720", "r1p0", true},
                                                      {"Mali-T760", "r0p0", true},
                                                      {"Mali-T760", "r0p1", true},
                                                      {"Mali-T760", "r0p1_50rel0", true},
                                                      {"Mali-T760", "r0p2", true},
                                                      {"Mali-T760", "r0p3", true},
                                                      {"Mali-T760", "r1p0", true}}}};
#endif

  std::vector<MaliDriverSet> const driversES3new = {
      {"Mali-T600_r3p0-00rel0",
       {{"Mali-T600", "r0p0", false},
        {"Mali-T600", "r0p0_15dev0", false},
        {"Mali-T600", "r0p1", false},
        {"Mali-T620", "r0p0", false},
        {"Mali-T620", "r0p1", false},
        {"Mali-T620", "r1p0", false}}},
      {"Mali-T600_r4p0-00rel0",
       {{"Mali-T600", "r0p0", true},
        {"Mali-T600", "r0p0_15dev0", true},
        {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true},
        {"Mali-T620", "r1p0", true}}},
      {"Mali-T600_r4p1-00rel0",
       {{"Mali-T600", "r0p0", true},
        {"Mali-T600", "r0p0_15dev0", true},
        {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true},
        {"Mali-T620", "r1p0", true},
        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true},
        {"Mali-T720", "r1p0", true},
        {"Mali-T760", "r0p0", true},
        {"Mali-T760", "r0p1", true},
        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true},
        {"Mali-T760", "r0p3", true},
        {"Mali-T760", "r1p0", true}}},
      {"Mali-T600_r5p0-00rel0",
       {{"Mali-T600", "r0p0", true},
        {"Mali-T600", "r0p0_15dev0", true},
        {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true},
        {"Mali-T620", "r1p0", true},
        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true},
        {"Mali-T720", "r1p0", true},
        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true},
        {"Mali-T760", "r0p1", true},
        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true},
        {"Mali-T760", "r0p3", true},
        {"Mali-T760", "r1p0", true}}},
      {"Mali-T600_r5p1-00rel0",
       {{"Mali-T600", "r0p0", true},
        {"Mali-T600", "r0p0_15dev0", true},
        {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true},
        {"Mali-T620", "r1p0", true},
        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true},
        {"Mali-T720", "r1p0", true},
        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true},
        {"Mali-T760", "r0p1", true},
        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true},
        {"Mali-T760", "r0p3", true},
        {"Mali-T760", "r1p0", true},
        {"Mali-T860", "r0p2", true},
        {"Mali-T880", "r0p0", true},
        {"Mali-T880", "r0p1", true},
        {"Mali-T880", "r0p2", true}}},
      {"Mali-T600_r6p0-00rel0",
       {{"Mali-T600", "r0p0", true}, {"Mali-T600", "r0p0_15dev0", true}, {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true}, {"Mali-T620", "r1p0", true},        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true}, {"Mali-T720", "r1p0", true},        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true}, {"Mali-T760", "r0p1", true},        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true}, {"Mali-T760", "r0p3", true},        {"Mali-T760", "r1p0", true},
        {"Mali-T820", "r0p0", true}, {"Mali-T830", "r1p0", true},        {"Mali-T830", "r0p1", true},
        {"Mali-T860", "r0p2", true}, {"Mali-T860", "r1p0", true},        {"Mali-T880", "r1p0", true},
        {"Mali-T880", "r0p2", true}, {"Mali-T880", "r0p1", true}}},
      {"Mali-T600_r7p0-00rel0",
       {{"Mali-T600", "r0p0", true}, {"Mali-T600", "r0p0_15dev0", true}, {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true}, {"Mali-T620", "r1p0", true},        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true}, {"Mali-T720", "r1p0", true},        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true}, {"Mali-T760", "r0p1", true},        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true}, {"Mali-T760", "r0p3", true},        {"Mali-T760", "r1p0", true},
        {"Mali-T820", "r0p0", true}, {"Mali-T820", "r0p1", true},        {"Mali-T820", "r1p0", true},
        {"Mali-T830", "r1p0", true}, {"Mali-T830", "r0p1", true},        {"Mali-T860", "r0p2", true},
        {"Mali-T860", "r1p0", true}, {"Mali-T860", "r2p0", true},        {"Mali-T880", "r1p0", true},
        {"Mali-T880", "r0p2", true}, {"Mali-T880", "r0p1", true},        {"Mali-T880", "r2p0", true}}},
      {"Mali-T600_r8p0-00rel0",
       {{"Mali-T600", "r0p0", true}, {"Mali-T600", "r0p0_15dev0", true}, {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true}, {"Mali-T620", "r1p0", true},        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true}, {"Mali-T720", "r1p0", true},        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true}, {"Mali-T760", "r0p1", true},        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true}, {"Mali-T760", "r0p3", true},        {"Mali-T760", "r1p0", true},
        {"Mali-T820", "r0p0", true}, {"Mali-T820", "r0p1", true},        {"Mali-T820", "r1p0", true},
        {"Mali-T830", "r1p0", true}, {"Mali-T830", "r0p1", true},        {"Mali-T860", "r0p2", true},
        {"Mali-T860", "r1p0", true}, {"Mali-T860", "r2p0", true},        {"Mali-T880", "r1p0", true},
        {"Mali-T880", "r0p2", true}, {"Mali-T880", "r0p1", true},        {"Mali-T880", "r2p0", true}}},
      {"Mali-T600_r9p0-00rel0",
       {{"Mali-T600", "r0p0", true}, {"Mali-T600", "r0p0_15dev0", true}, {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true}, {"Mali-T620", "r1p0", true},        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true}, {"Mali-T720", "r1p0", true},        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true}, {"Mali-T760", "r0p1", true},        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true}, {"Mali-T760", "r0p3", true},        {"Mali-T760", "r1p0", true},
        {"Mali-T820", "r0p0", true}, {"Mali-T820", "r0p1", true},        {"Mali-T820", "r1p0", true},
        {"Mali-T830", "r1p0", true}, {"Mali-T830", "r0p1", true},        {"Mali-T860", "r0p2", true},
        {"Mali-T860", "r1p0", true}, {"Mali-T860", "r2p0", true},        {"Mali-T880", "r1p0", true},
        {"Mali-T880", "r0p2", true}, {"Mali-T880", "r0p1", true},        {"Mali-T880", "r2p0", true}}},
      {"Mali-T600_r10p0-00rel0",
       {{"Mali-T600", "r0p0", true}, {"Mali-T600", "r0p0_15dev0", true}, {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true}, {"Mali-T620", "r1p0", true},        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true}, {"Mali-T720", "r1p0", true},        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true}, {"Mali-T760", "r0p1", true},        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true}, {"Mali-T760", "r0p3", true},        {"Mali-T760", "r1p0", true},
        {"Mali-T820", "r0p0", true}, {"Mali-T820", "r0p1", true},        {"Mali-T820", "r1p0", true},
        {"Mali-T830", "r1p0", true}, {"Mali-T830", "r0p1", true},        {"Mali-T860", "r0p2", true},
        {"Mali-T860", "r1p0", true}, {"Mali-T860", "r2p0", true},        {"Mali-T880", "r1p0", true},
        {"Mali-T880", "r0p2", true}, {"Mali-T880", "r0p1", true},        {"Mali-T880", "r2p0", true}}},
      {"Mali-T600_r11p0-00rel0",
       {{"Mali-T600", "r0p0", true}, {"Mali-T600", "r0p0_15dev0", true}, {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true}, {"Mali-T620", "r1p0", true},        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true}, {"Mali-T720", "r1p0", true},        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true}, {"Mali-T760", "r0p1", true},        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true}, {"Mali-T760", "r0p3", true},        {"Mali-T760", "r1p0", true},
        {"Mali-T820", "r0p0", true}, {"Mali-T820", "r0p1", true},        {"Mali-T820", "r1p0", true},
        {"Mali-T830", "r1p0", true}, {"Mali-T830", "r0p1", true},        {"Mali-T860", "r0p2", true},
        {"Mali-T860", "r1p0", true}, {"Mali-T860", "r2p0", true},        {"Mali-T880", "r1p0", true},
        {"Mali-T880", "r0p2", true}, {"Mali-T880", "r0p1", true},        {"Mali-T880", "r2p0", true}}},
      {"Mali-T600_r12p0-00rel0",
       {{"Mali-T600", "r0p0", true}, {"Mali-T600", "r0p0_15dev0", true}, {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true}, {"Mali-T620", "r1p0", true},        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true}, {"Mali-T720", "r1p0", true},        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true}, {"Mali-T760", "r0p1", true},        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true}, {"Mali-T760", "r0p3", true},        {"Mali-T760", "r1p0", true},
        {"Mali-T820", "r0p0", true}, {"Mali-T820", "r0p1", true},        {"Mali-T820", "r1p0", true},
        {"Mali-T830", "r1p0", true}, {"Mali-T830", "r0p1", true},        {"Mali-T860", "r0p2", true},
        {"Mali-T860", "r1p0", true}, {"Mali-T860", "r2p0", true},        {"Mali-T880", "r1p0", true},
        {"Mali-T880", "r0p2", true}, {"Mali-T880", "r0p1", true},        {"Mali-T880", "r2p0", true}}},
      {"Mali-T600_r13p0-00rel0",
       {{"Mali-T600", "r0p0", true}, {"Mali-T600", "r0p0_15dev0", true}, {"Mali-T600", "r0p1", true},
        {"Mali-T620", "r0p1", true}, {"Mali-T620", "r1p0", true},        {"Mali-T620", "r1p1", true},
        {"Mali-T720", "r0p0", true}, {"Mali-T720", "r1p0", true},        {"Mali-T720", "r1p1", true},
        {"Mali-T760", "r0p0", true}, {"Mali-T760", "r0p1", true},        {"Mali-T760", "r0p1_50rel0", true},
        {"Mali-T760", "r0p2", true}, {"Mali-T760", "r0p3", true},        {"Mali-T760", "r1p0", true},
        {"Mali-T820", "r0p0", true}, {"Mali-T820", "r0p1", true},        {"Mali-T820", "r1p0", true},
        {"Mali-T830", "r1p0", true}, {"Mali-T830", "r0p1", true},        {"Mali-T860", "r0p2", true},
        {"Mali-T860", "r1p0", true}, {"Mali-T860", "r2p0", true},        {"Mali-T880", "r1p0", true},
        {"Mali-T880", "r0p2", true}, {"Mali-T880", "r0p1", true},        {"Mali-T880", "r2p0", true}}},
      {"Mali-Gxx_r3p0-00rel0", {{"Mali-G71", "r0p0", false}}}};

  std::vector<MaliDriverSet> driversES2new = {{"Mali-400_r5p0-01rel0",
                                               {{"Mali-300", "r0p0", true},
                                                {"Mali-400", "r1p1", true},
                                                {"Mali-400", "r1p0", true},
                                                {"Mali-400", "r0p1", true},
                                                {"Mali-400", "r0p0", true},
                                                {"Mali-450", "r0p0", true}}},
                                              {"Mali-400_r6p1-00rel0",
                                               {{"Mali-400", "r1p1", true},
                                                {"Mali-400", "r1p0", true},
                                                {"Mali-400", "r0p1", true},
                                                {"Mali-400", "r0p0", true},
                                                {"Mali-450", "r0p0", true},
                                                {"Mali-470", "r0p1", true}}},
                                              {"Mali-400_r7p0-00rel0",
                                               {{"Mali-400", "r1p1", true},
                                                {"Mali-400", "r1p0", true},
                                                {"Mali-400", "r0p1", true},
                                                {"Mali-400", "r0p0", true},
                                                {"Mali-450", "r0p0", true},
                                                {"Mali-470", "r0p1", true}}}};
  driversES2new.insert(driversES2new.end(), driversES3new.begin(), driversES3new.end());

  std::vector<MaliCompilerData> const compilers = {{dp::ApiVersion::OpenGLES3, GetCompilerPath(kCompilerMaliOpenGLES3),
                                                    GetCompilerPath(kMaliCompilerOpenGLES3Dir), driversES3new}};

  base::DelayedThreadPool workerThread(16 /* threadsCount */);
  uint32_t counter = 0;
  std::atomic<uint32_t> progressCounter(0);
  for (auto const & compiler : compilers)
  {
    for (auto const & set : compiler.m_driverSets)
    {
      for (auto const & version : set.m_releases)
      {
#if defined(OMIM_OS_MAC)
        if (!version.m_availableForMacOS)
          continue;
#endif
        counter++;
        workerThread.Push([&progressCounter, compiler, set, version]
        {
          MaliCompileShaders(compiler, set, version);
          progressCounter++;
        });
      }
    }
  }

  // Here we are in active waiting, because thread_pool::delayed::ThreadPool stops dispatching tasks
  // to different threads in case of shutting down.
  while (progressCounter < counter)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  workerThread.Shutdown(base::DelayedThreadPool::Exit::ExecPending);
}
