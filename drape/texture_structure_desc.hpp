#pragma once

#include "../geometry/rect2d.hpp"

#include "../std/map.hpp"
#include "../std/stdint.hpp"

class TextureStructureDesc
{
public:
  void Load(const string & descFilePath, uint32_t & width, uint32_t & height);
  bool GetResource(string const & name, m2::RectU & rect) const;

private:
  typedef map<string, m2::RectU> structure_t;
  structure_t m_structure;
};
