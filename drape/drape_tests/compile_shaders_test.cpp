#include "../../testing/testing.hpp"

#include "enum_shaders.hpp"

#include "../../platform/platform.hpp"
#include "../../std/sstream.hpp"
#include "../../std/target_os.hpp"

#include <QtCore/QProcess>
#include <QtCore/QTextStream>

#if defined (OMIM_OS_MAC)
  #define SHADERS_COMPILER "GLSLESCompiler_Series5.mac"
#elif defined (OMIM_OS_LINUX)
  #define SHADERS_COMPILER "GLSLESCompiler_Series5"
#elif defined (OMIM_OS_WINDOWS)
  #define SHADERS_COMPILER "GLSLESCompiler_Series5.exe"
#else
  #error "Define shaders compiler for your platform"
#endif


string DebugPrint(QString const & s)
{
  return s.toStdString();
}

UNIT_TEST(CompileShaders_Test)
{
  Platform & platform = GetPlatform();
  string glslCompilerPath = platform.ResourcesDir() + "shaders_compiler/" SHADERS_COMPILER;
  if (!platform.IsFileExistsByFullPath(glslCompilerPath))
  {
    glslCompilerPath = platform.WritableDir() + "shaders_compiler/" SHADERS_COMPILER;
    if (!platform.IsFileExistsByFullPath(glslCompilerPath))
      TEST_EQUAL(false, true, ("GLSL compiler not found"));
  }

  bool isPass = true;
  QString errorLog;
  QTextStream ss(&errorLog);

  gpu_test::InitEnumeration();
  for (size_t i = 0; i < gpu_test::VertexEnum.size(); ++i)
  {
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;
    args << QString::fromStdString(gpu_test::VertexEnum[i])
         << QString::fromStdString(gpu_test::VertexEnum[i] + ".bin")
         << "-v";
    p.start(QString::fromStdString(glslCompilerPath), args, QIODevice::ReadOnly);

    if (!p.waitForStarted())
      TEST_EQUAL(false, true, ("GLSL compiler not started"));

    if (!p.waitForFinished())
      TEST_EQUAL(false, true, ("GLSL compiler not finished in time"));

    QString result = p.readAllStandardOutput();
    if (result.indexOf("Success") == -1)
    {
      ss << "\n" << QString("SHADER COMPILE ERROR\n");
      ss << QString::fromStdString(gpu_test::VertexEnum[i]) << "\n";
      ss << result.trimmed() << "\n";
      isPass = false;
    }
  }

  for (size_t i = 0; i < gpu_test::FragmentEnum.size(); ++i)
  {
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;
    args << QString::fromStdString(gpu_test::FragmentEnum[i])
         << QString::fromStdString(gpu_test::FragmentEnum[i] + ".bin")
         << "-f";
    p.start(QString::fromStdString(glslCompilerPath), args, QIODevice::ReadOnly);

    if (!p.waitForStarted())
      TEST_EQUAL(false, true, ("GLSL compiler not started"));

    if (!p.waitForFinished())
      TEST_EQUAL(false, true, ("GLSL compiler not finished in time"));

    QString result = p.readAllStandardOutput();
    if (result.indexOf("Succes") == -1)
    {
      ss << "\n" << QString("SHADER COMPILE ERROR\n");
      ss << QString::fromStdString(gpu_test::FragmentEnum[i]) << "\n";
      ss << result.trimmed() << "\n";
      isPass = false;
    }
  }

  TEST_EQUAL(isPass, true, (errorLog));
}
