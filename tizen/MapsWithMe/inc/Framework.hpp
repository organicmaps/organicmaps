#pragma once
#include "../../../std/stdint.hpp"
#include "../../../std/noncopyable.hpp"
#include "../../../std/shared_ptr.hpp"

class Framework;
namespace Tizen { namespace Ui { namespace Controls
{
class Form;
}}}

namespace tizen
{
class RenderContext;
class VideoTimer1;

class Framework : public noncopyable
{
public:
  Framework(Tizen::Ui::Controls::Form * form);
  virtual ~Framework();
  static ::Framework * GetInstance();
  void Draw();

private:
  static ::Framework * m_Instance;
  VideoTimer1 * m_VideoTimer;
  shared_ptr<RenderContext> m_context;
};

}
