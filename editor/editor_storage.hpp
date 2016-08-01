#pragma once

#include "std/unique_ptr.hpp"

namespace pugi
{
class xml_document;
}

namespace editor
{
// Editor storage interface.
class StorageBase
{
public:
  virtual ~StorageBase() {}
  virtual bool Save(pugi::xml_document const & doc) = 0;
  virtual bool Load(pugi::xml_document & doc) = 0;
  virtual void Reset() = 0;
};

// Class which save/load edits to/from local file.
class StorageLocal : public StorageBase
{
public:
  bool Save(pugi::xml_document const & doc) override;
  bool Load(pugi::xml_document & doc) override;
  void Reset() override;
};

// Class which save/load edits to/from xml_document class instance.
class StorageMemory : public StorageBase
{
public:
  StorageMemory();
  bool Save(pugi::xml_document const & doc) override;
  bool Load(pugi::xml_document & doc) override;
  void Reset() override;

private:
  unique_ptr<pugi::xml_document> m_doc;
};
}
