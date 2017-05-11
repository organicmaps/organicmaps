#include "testing/testing.hpp"

#include "drape/shader_def.hpp"
#include "drape/shader.hpp"

#include "drape/glconstants.hpp"

#include "drape/drape_tests/glmock_functions.hpp"

#include "base/scope_guard.hpp"
#include "platform/platform.hpp"

#include "std/sstream.hpp"
#include "std/target_os.hpp"
#include "std/vector.hpp"
#include "std/string.hpp"

#include <QtCore/QProcess>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QTemporaryFile>

#include <gmock/gmock.h>

using testing::Return;
using testing::AnyNumber;
using namespace dp;

#if defined (OMIM_OS_MAC)
  #define SHADERS_COMPILER "GLSLESCompiler_Series5.mac"
  #define MALI_SHADERS_COMPILER "mali_compiler/malisc"
  #define MALI_DIR "mali_compiler/"
#elif defined (OMIM_OS_LINUX)
  #define SHADERS_COMPILER "GLSLESCompiler_Series5"
  #define MALI_SHADERS_COMPILER "mali_compiler/malisc"
  #define MALI_DIR "mali_compiler/"
#elif defined (OMIM_OS_WINDOWS)
  #define SHADERS_COMPILER "GLSLESCompiler_Series5.exe"
#else
  #error "Define shaders compiler for your platform"
#endif


string DebugPrint(QString const & s)
{
  return s.toStdString();
}

struct ShaderEnumGuard
{
  ShaderEnumGuard()
  {
    gpu::InitEnumeration();
  }

  ~ShaderEnumGuard()
  {
    gpu::VertexEnum.clear();
    gpu::FragmentEnum.clear();
  }
};

void WriteShaderToFile(QTemporaryFile & file, string shader)
{
  EXPECTGL(glGetInteger(gl_const::GLMaxFragmentTextures)).WillRepeatedly(Return(8));
  PreprocessShaderSource(shader);
  QTextStream out(&file);
  out << QString::fromStdString(shader);
}

typedef function<void (QProcess & p)> TPrepareProcessFn;
typedef function<void (QStringList & args, QString const & fileName)> TPrepareArgumentsFn;
typedef function<bool (QString const & output)> TSuccessComparator;

void RunShaderTest(QString const & glslCompiler,
                   QString const & fileName,
                   TPrepareProcessFn const & procPrepare,
                   TPrepareArgumentsFn const & argsPrepare,
                   TSuccessComparator const & successComparator,
                   QTextStream & errorLog)
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
    errorLog << fileName << "\n";
    errorLog << result.trimmed() << "\n";
  }
}

void ForEachShader(string const & defines,
                   vector<string> const & shaders,
                   QString const & glslCompiler,
                   TPrepareProcessFn const & procPrepare,
                   TPrepareArgumentsFn const & argsPrepare,
                   TSuccessComparator const & successComparator,
                   QTextStream & errorLog)
{
  for (string src : shaders)
  {
    QTemporaryFile srcFile;
    TEST(srcFile.open(), ("Temporary File can't be created!"));
    string fullSrc = defines + src;
    WriteShaderToFile(srcFile, fullSrc);
    RunShaderTest(glslCompiler, srcFile.fileName(),
                  procPrepare, argsPrepare, successComparator, errorLog);
  }
}

UNIT_TEST(CompileShaders_Test)
{
  Platform & platform = GetPlatform();
  string glslCompilerPath = platform.ResourcesDir() + "shaders_compiler/" SHADERS_COMPILER;
  if (!platform.IsFileExistsByFullPath(glslCompilerPath))
  {
    glslCompilerPath = platform.WritableDir() + "shaders_compiler/" SHADERS_COMPILER;
    TEST(platform.IsFileExistsByFullPath(glslCompilerPath), ("GLSL compiler not found"));
  }

  QString errorLog;
  QTextStream ss(&errorLog);

  ShaderEnumGuard guard;
  QString compilerPath = QString::fromStdString(glslCompilerPath);
  QString shaderType = "-v";
  auto argsPrepareFn = [&shaderType] (QStringList & args, QString const & fileName)
                       {
                         args << fileName
                              << fileName + ".bin"
                              << shaderType;
                       };
  auto successComparator = [] (QString const & output) { return output.indexOf("Success") != -1; };

  string defines = "";
  ForEachShader(defines, gpu::VertexEnum, compilerPath, [] (QProcess const &) {},
                argsPrepareFn, successComparator, ss);
  shaderType = "-f";
  ForEachShader(defines, gpu::FragmentEnum, compilerPath,[] (QProcess const &) {},
                argsPrepareFn, successComparator, ss);

  TEST_EQUAL(errorLog.isEmpty(), true, ("PVR without defines :", errorLog));

  defines = "#define ENABLE_VTF\n";
  errorLog.clear();
  shaderType = "-v";
  ForEachShader(defines, gpu::VertexEnum, compilerPath, [] (QProcess const &) {},
                argsPrepareFn, successComparator, ss);
  shaderType = "-f";
  ForEachShader(defines, gpu::FragmentEnum, compilerPath,[] (QProcess const &) {},
                argsPrepareFn, successComparator, ss);

  TEST_EQUAL(errorLog.isEmpty(), true, ("PVR with defines : ", defines, "\n", errorLog));

  defines = "#define SAMSUNG_GOOGLE_NEXUS\n";
  errorLog.clear();
  shaderType = "-v";
  ForEachShader(defines, gpu::VertexEnum, compilerPath, [] (QProcess const &) {},
                argsPrepareFn, successComparator, ss);
  shaderType = "-f";
  ForEachShader(defines, gpu::FragmentEnum, compilerPath,[] (QProcess const &) {},
                argsPrepareFn, successComparator, ss);

  TEST_EQUAL(errorLog.isEmpty(), true, ("PVR with defines : ", defines, "\n", errorLog));
}

#ifdef OMIM_OS_MAC

void TestMaliShaders(QString const & driver,
                     QString const & hardware,
                     QString const & release)
{
  Platform & platform = GetPlatform();
  string glslCompilerPath = platform.ResourcesDir() + "shaders_compiler/" MALI_SHADERS_COMPILER;
  TEST(platform.IsFileExistsByFullPath(glslCompilerPath), ("GLSL MALI compiler not found"));

  QString errorLog;
  QTextStream ss(&errorLog);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("MALICM_LOCATION", QString::fromStdString(platform.ResourcesDir() + "shaders_compiler/" MALI_DIR));
  auto procPrepare = [&env] (QProcess & p) { p.setProcessEnvironment(env); };
  QString shaderType = "-v";
  auto argForming = [&] (QStringList & args, QString const & fileName)
  {
    args << shaderType
         << "-V"
         << "-r"
         << release
         << "-c"
         << hardware
         << "-d"
         << driver
         << fileName;
  };

  auto succesComparator = [] (QString const & output)
                            {
                              return output.indexOf("Compilation succeeded.") != -1;
                            };

  string defines = "";
  QString const compilerPath = QString::fromStdString(glslCompilerPath);
  ForEachShader(defines, gpu::VertexEnum, compilerPath, procPrepare, argForming, succesComparator, ss);
  shaderType = "-f";
  ForEachShader(defines, gpu::FragmentEnum, compilerPath, procPrepare, argForming, succesComparator, ss);
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

  ShaderEnumGuard guard;

  for (DriverSet set : models)
  {
    for (TReleaseVersion version : set.m_releases)
      TestMaliShaders(set.m_driverName, version.first, version.second);
  }
}

#endif
