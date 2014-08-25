#include "../../testing/testing.hpp"

#include "../../drape/shader_def.hpp"
#include "../../drape/shader.hpp"

#include "../glconstants.hpp"

#include "glmock_functions.hpp"

#include "../../platform/platform.hpp"
#include "../../std/sstream.hpp"
#include "../../std/target_os.hpp"
#include "../../std/vector.hpp"
#include "../../std/string.hpp"

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

void WriteShaderToFile(QTemporaryFile & file, string shader)
{
  EXPECTGL(glGetInteger(gl_const::GLMaxFragmentTextures)).WillRepeatedly(Return(8));
  sh::Inject(shader);
  QTextStream out(&file);
  out << QString::fromStdString(shader);
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

  bool isPass = true;
  QString errorLog;
  QTextStream ss(&errorLog);

  gpu::InitEnumeration();

  for (size_t i = 0; i < gpu::VertexEnum.size(); ++i)
  {
    QTemporaryFile file;
    if (file.open())
      WriteShaderToFile(file, gpu::VertexEnum[i]);
    else
      TEST(false, ("File can't be created!"));

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;

    args << file.fileName()
         << file.fileName() + ".bin"
         << "-v";
    p.start(QString::fromStdString(glslCompilerPath), args, QIODevice::ReadOnly);

    TEST(p.waitForStarted(), ("GLSL compiler not started"));
    TEST(p.waitForFinished(), ("GLSL compiler not finished in time"));

    QString result = p.readAllStandardOutput();
    if (result.indexOf("Success") == -1)
    {
      ss << "\n" << QString("SHADER COMPILE ERROR\n");
      ss << file.fileName() << "\n";
      ss << result.trimmed() << "\n";
      isPass = false;
    }
  }

  for (size_t i = 0; i < gpu::FragmentEnum.size(); ++i)
  {
    QTemporaryFile file;
    if (file.open())
      WriteShaderToFile(file, gpu::FragmentEnum[i]);
    else
      TEST(false, ("File can't be created!"));
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;
    args << file.fileName()
         << file.fileName() + ".bin"
         << "-f";
    p.start(QString::fromStdString(glslCompilerPath), args, QIODevice::ReadOnly);

    TEST(p.waitForStarted(), ("GLSL compiler not started"));
    TEST(p.waitForFinished(), ("GLSL compiler not finished in time"));

    QString result = p.readAllStandardOutput();
    if (result.indexOf("Succes") == -1)
    {
      ss << "\n" << QString("SHADER COMPILE ERROR\n");
      ss << file.fileName() << "\n";
      ss << result.trimmed() << "\n";
      isPass = false;
    }
  }

  TEST(isPass, (errorLog));

  gpu::VertexEnum.clear();
  gpu::FragmentEnum.clear();
}

#ifdef OMIM_OS_MAC

void TestMaliShaders(string driver, string hardware, string release)
{
  Platform & platform = GetPlatform();
  string glslCompilerPath = platform.ResourcesDir() + "shaders_compiler/" MALI_SHADERS_COMPILER;
  TEST(platform.IsFileExistsByFullPath(glslCompilerPath), ("GLSL MALI compiler not found"));

  bool isPass = true;
  QString errorLog;
  QTextStream ss(&errorLog);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("MALICM_LOCATION", QString::fromStdString(platform.ResourcesDir() + "shaders_compiler/" MALI_DIR));

  for (size_t i = 0; i < gpu::VertexEnum.size(); ++i)
  {
    QTemporaryFile file;
    if (file.open())
      WriteShaderToFile(file, gpu::VertexEnum[i]);
    else
      TEST(false, ("File can't be created!"));

    QProcess p;
    p.setProcessEnvironment(env);
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;
    args << "-v"
         << "-V"
         << "-r"
         << QString::fromStdString(release)
         << "-c"
         << QString::fromStdString(hardware)
         << "-d"
         << QString::fromStdString(driver)
         << file.fileName();
    p.start(QString::fromStdString(glslCompilerPath), args, QIODevice::ReadOnly);

    TEST(p.waitForStarted(), ("GLSL compiler not started"));
    TEST(p.waitForFinished(), ("GLSL compiler not finished in time"));

    QString result = p.readAllStandardOutput();
    if (result.indexOf("Compilation succeeded.") == -1)
    {
      ss << "\n" << QString("SHADER COMPILE ERROR\n");
      ss << file.fileName() << "\n";
      ss << result.trimmed() << "\n";
      isPass = false;
    }
  }

  for (size_t i = 0; i < gpu::FragmentEnum.size(); ++i)
  {
    QTemporaryFile file;
    if (file.open())
      WriteShaderToFile(file, gpu::FragmentEnum[i]);
    else
      TEST(false, ("File can't be created!"));

    QProcess p;
    p.setProcessEnvironment(env);
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;
    args << "-f"
         << "-V"
         << "-r"
         << QString::fromStdString(release)
         << "-c"
         << QString::fromStdString(hardware)
         << "-d"
         << QString::fromStdString(driver)
         << file.fileName();
    p.start(QString::fromStdString(glslCompilerPath), args, QIODevice::ReadOnly);

    TEST(p.waitForStarted(), ("GLSL compiler not started"));
    TEST(p.waitForFinished(), ("GLSL compiler not finished in time"));

    QString result = p.readAllStandardOutput();
    if (result.indexOf("Compilation succeeded.") == -1)
    {
      ss << "\n" << QString("SHADER COMPILE ERROR\n");
      ss << file.fileName() << "\n";
      ss << result.trimmed() << "\n";
      isPass = false;
    }
  }

  TEST(isPass, (errorLog));
}

UNIT_TEST(MALI_CompileShaders_Test)
{
  struct driver_set
  {
    vector<std::pair<string, string> > m_releases;
    string m_driverName;
  };

  vector<driver_set> models(3);
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

  gpu::InitEnumeration();

  for(size_t i = 0; i < models.size(); ++i)
    for(size_t j = 0; j < models[i].m_releases.size(); ++j)
      TestMaliShaders(models[i].m_driverName, models[i].m_releases[j].first, models[i].m_releases[j].second);

  gpu::VertexEnum.clear();
  gpu::FragmentEnum.clear();
}

#endif
