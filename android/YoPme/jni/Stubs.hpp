#include "../../../graphics/opengl/gl_render_context.hpp"
#include "../../../platform/video_timer.hpp"

namespace yopme
{

class RenderContext: public graphics::gl::RenderContext
{
public:
  virtual void makeCurrent();
  virtual graphics::RenderContext * createShared();
};

class EmptyVideoTimer: public VideoTimer
{
  typedef VideoTimer base_t;
public:
  EmptyVideoTimer();
  ~EmptyVideoTimer();

  void start();
  void resume();
  void pause();
  void stop();
  void perform();
};

} // namespace yopme
