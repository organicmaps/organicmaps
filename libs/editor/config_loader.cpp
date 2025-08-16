#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"

#include "platform/platform.hpp"

#include <stdexcept>

#include "coding/file_reader.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"


namespace editor
{
using std::string;

namespace
{

constexpr char kConfigFileName[] = "editor.json";

}  // namespace

void Waiter::Interrupt()
{
  {
    std::lock_guard lock(m_mutex);
    m_interrupted = true;
  }
  m_event.notify_all();
}

ConfigLoader::ConfigLoader(base::AtomicSharedPtr<EditorConfig> & config) : m_config(config)
{
  ResetConfig(LoadFromLocal());
}

ConfigLoader::~ConfigLoader()
{
  m_waiter.Interrupt();
}

void ConfigLoader::ResetConfig(std::string_view buffer)
{
  auto config = std::make_shared<EditorConfig>();
  config->SetConfig(buffer);
  m_config.Set(config);
}

// static
std::string ConfigLoader::LoadFromLocal()
{
  string content;
  std::unique_ptr<ModelReader> reader;

  try
  {
    // Get config file from WritableDir first.
    reader = GetPlatform().GetReader(kConfigFileName, "wr");
  }
  catch (RootException const & ex)
  {
    LOG(LERROR, ("Failed to read editor config:", ex.Msg()));
  }

  if (reader)
    reader->ReadAsString(content);
  
  return content;
}
}  // namespace editor