#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"

#include "platform/platform.hpp"

#include <stdexcept>

#include "coding/file_reader.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "cppjansson/cppjansson.hpp"

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
  base::Json doc;
  LoadFromLocal(doc);
  ResetConfig(doc);
}

ConfigLoader::~ConfigLoader()
{
  m_waiter.Interrupt();
}

void ConfigLoader::ResetConfig(base::Json const & doc)
{
  auto config = std::make_shared<EditorConfig>();
  config->SetConfig(doc);
  m_config.Set(config);
}

// static
void ConfigLoader::LoadFromLocal(base::Json & doc)
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
    LOG(LERROR, (ex.Msg()));
    return;
  }

  if (reader)
    reader->ReadAsString(content);

  base::Json parsedDoc(content.c_str());

  if (!parsedDoc.get())
  {
    LOG(LERROR, (kConfigFileName, "can not be parsed as a valid JSON."));
    doc = base::Json();
  }
  else
  {
    // move the parsed data into the output param
    doc = std::move(parsedDoc);
  }
}
}  // namespace editor