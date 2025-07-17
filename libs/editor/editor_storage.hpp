#pragma once

#include <mutex>

#include <pugixml.hpp>

namespace editor
{
// Editor storage interface.
class StorageBase
{
public:
  virtual ~StorageBase() = default;

  virtual bool Save(pugi::xml_document const & doc) = 0;
  virtual bool Load(pugi::xml_document & doc) = 0;
  virtual bool Reset() = 0;
};

// Class which saves/loads edits to/from local file.
// Note: this class IS thread-safe.
class LocalStorage : public StorageBase
{
public:
  // StorageBase overrides:
  bool Save(pugi::xml_document const & doc) override;
  bool Load(pugi::xml_document & doc) override;
  bool Reset() override;

private:
  std::mutex m_mutex;
};

// Class which saves/loads edits to/from xml_document class instance.
// Note: this class is NOT thread-safe.
class InMemoryStorage : public StorageBase
{
public:
  // StorageBase overrides:
  bool Save(pugi::xml_document const & doc) override;
  bool Load(pugi::xml_document & doc) override;
  bool Reset() override;

private:
  pugi::xml_document m_doc;
};
}  // namespace editor
