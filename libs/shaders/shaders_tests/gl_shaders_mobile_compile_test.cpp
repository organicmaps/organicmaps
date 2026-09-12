#include "testing/testing.hpp"

#include "shaders/gl_shaders.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>

#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gl_shaders_mobile_compile_test
{
// Absolute path baked in by shaders_tests/CMakeLists.txt, so the test finds the compilers no matter
// which directory it is started from. The xcode/shaders project does not define it and resolves the
// compilers bundled into its Resources against the working directory.
#ifdef OMIM_SHADERS_COMPILERS_DIR
std::string const kCompilersDir = OMIM_SHADERS_COMPILERS_DIR;
#else
std::string const kCompilersDir = "shaders_compiler";
#endif

#if defined(OMIM_OS_MAC)
std::string const kMaliCompilerOpenGLES3Dir = "macos/mali_compiler_es3";
std::string const kCompilerMaliOpenGLES3 = kMaliCompilerOpenGLES3Dir + "/malisc";
std::string const kCompilerOpenGLES = "macos/glslangValidator";
#elif defined(OMIM_OS_LINUX)
std::string const kMaliCompilerOpenGLES3Dir = "linux/mali_compiler_es3";
std::string const kCompilerMaliOpenGLES3 = kMaliCompilerOpenGLES3Dir + "/malisc";
std::string const kCompilerOpenGLES = "linux/glslangValidator";
#endif

QString GetCompilerPath(std::string const & compilerName)
{
  std::string const compilerPath = base::JoinPath(kCompilersDir, compilerName);
  TEST(GetPlatform().IsFileExistsByFullPath(compilerPath), ("Shader compiler path not found:", compilerPath));
  return QString::fromStdString(compilerPath);
}

// Writes each shader of all programs once. The file extension tells the compilers the shader stage.
QStringList WriteShaders(QTemporaryDir const & dir, std::string const & defines)
{
  std::map<std::string, char const *> shaders;
  for (size_t i = 0; i < static_cast<size_t>(gpu::Program::ProgramsCount); ++i)
  {
    auto const & info = gpu::GetProgramInfo(dp::ApiVersion::OpenGLES3, static_cast<gpu::Program>(i));
    shaders[info.m_vertexShaderName + ".vert"] = info.m_vertexShaderSource;
    shaders[info.m_fragmentShaderName + ".frag"] = info.m_fragmentShaderSource;
  }

  QStringList paths;
  for (auto const & [name, source] : shaders)
  {
    paths << dir.filePath(QString::fromStdString(name));
    std::ofstream(paths.back().toStdString()) << gpu::GLES3_SHADER_VERSION << defines << source;
  }
  return paths;
}

// Returns the exit code and the merged stdout and stderr, or -1 and the error if the compiler did not finish.
std::pair<int, std::string> RunCompiler(QString const & compiler, QStringList const & args)
{
  QProcess p;
  p.setProcessChannelMode(QProcess::MergedChannels);
  p.start(compiler, args, QIODevice::ReadOnly);
  if (!p.waitForFinished(60000 /* msecs */) || p.exitStatus() != QProcess::NormalExit)
    return {-1, p.errorString().toStdString()};
  return {p.exitCode(), p.readAllStandardOutput().toStdString()};
}

UNIT_TEST(MobileCompileShaders_Test)
{
  QString const compiler = GetCompilerPath(kCompilerOpenGLES);
  for (char const * defines : {"", "#define ENABLE_VTF\n"})
  {
    QTemporaryDir const dir;
    TEST(dir.isValid(), ());
    QStringList const shaders = WriteShaders(dir, defines);

    // glslangValidator compiles the files separately and prints the name of each one before its errors and warnings.
    auto const [exitCode, output] = RunCompiler(compiler, shaders);
    TEST(exitCode == 0 && output == shaders.join('\n').toStdString() + '\n', ("Defines:", defines, "\n", output));
  }
}

UNIT_TEST(MALI_MobileCompileShaders_Test)
{
  QTemporaryDir const dir;
  TEST(dir.isValid(), ());
  // Mali GPUs do not support ENABLE_VTF.
  QStringList const shaders = WriteShaders(dir, {});

  QString const compiler = GetCompilerPath(kCompilerMaliOpenGLES3);
  QString const compilerDir = GetCompilerPath(kMaliCompilerOpenGLES3Dir);
  qputenv("MALICM_LOCATION", compilerDir.toUtf8());

  // Each bundled driver library is a separate compiler. Without -c and -r malisc targets the newest GPU core and
  // revision of the driver, which affect only the generated code, not whether a shader compiles.
  Platform::FilesList drivers;
  Platform::GetFilesByRegExp(base::JoinPath(compilerDir.toStdString(), "openglessl"), R"(^libMali-.+\.(so|dylib)$)",
                             drivers);
  TEST(!drivers.empty(), ());

  // Unlike glslangValidator, malisc concatenates its input files into one shader, so it runs once per shader.
  // The drivers are independent, so each one runs in its own thread and collects its own errors.
  std::vector<std::string> errors(drivers.size());
  std::vector<std::thread> threads;
  for (size_t i = 0; i < drivers.size(); ++i)
  {
    threads.emplace_back([&, i]
    {
      // libMali-T600_r13p0-00rel0.so -> Mali-T600_r13p0-00rel0
      QString const driver = QString::fromStdString(base::FilenameWithoutExt(drivers[i]).substr(3));
      for (auto const & shader : shaders)
      {
        auto const [exitCode, output] = RunCompiler(compiler, {"-d", driver, shader});
        if (exitCode != 0)
          errors[i] += "\n" + drivers[i] + ": " + shader.toStdString() + ":\n" + output;
      }
    });
  }
  for (auto & thread : threads)
    thread.join();

  std::string log;
  for (auto const & error : errors)
    log += error;
  TEST(log.empty(), (log));
}
}  // namespace gl_shaders_mobile_compile_test
