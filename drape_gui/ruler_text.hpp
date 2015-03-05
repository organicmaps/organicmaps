#pragma once

#include "shape.hpp"
#include "gui_text.hpp"

namespace gui
{

class RulerText : public Shape
{
public:
  RulerText();

  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureManager> tex) const;
  virtual uint16_t GetVertexCount() const;
  virtual uint16_t GetIndexCount() const;

private:
  string m_alphabet;
  size_t m_maxLength;
};

}
