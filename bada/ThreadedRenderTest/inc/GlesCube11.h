#ifndef _GLESCUBE11_H_
#define _GLESCUBE11_H_


#include <FBase.h>
#include <FGraphics.h>
#include <FApp.h>
#include <FGraphicsOpengl.h>
#include <FSystem.h>
#include <FUi.h>

class ThreadRenderer;

class GlesCube11 : 
	public Osp::App::Application,
	public Osp::System::IScreenEventListener,
	public Osp::Base::Runtime::ITimerEventListener
{
public:
	static Osp::App::Application* CreateInstance(void);

	ThreadRenderer * m_renderer;

	Osp::Base::Runtime::Monitor * m_pbufferMonitor;
	Osp::Base::Runtime::Monitor * m_primaryMonitor;

	GlesCube11();
	~GlesCube11();

	bool OnAppInitializing(Osp::App::AppRegistry& appRegistry);
	bool OnAppTerminating(Osp::App::AppRegistry& appRegistry, bool forcedTermination = false);
	void OnForeground(void);
	void OnBackground(void);
	void OnLowMemory(void);
	void OnBatteryLevelChanged(Osp::System::BatteryLevel batteryLevel);
	void OnScreenOn (void);
	void OnScreenOff (void);

	void OnTimerExpired(Osp::Base::Runtime::Timer& timer);
	bool Draw();

	bool InitEGL();
	bool InitGL();
	void Cleanup();
	void DestroyGL();
	void DrawCube1();
	void DrawCube2();
	void DrawTexture2D(int x, int y, int w, int h, Osp::Graphics::Opengl::GLuint texture, int texWidth, int texHeight);

	Osp::Graphics::Opengl::EGLDisplay __eglDisplay;
	Osp::Graphics::Opengl::EGLSurface __eglSurface;
	Osp::Graphics::Opengl::EGLConfig  __eglConfig;
	Osp::Graphics::Opengl::EGLContext __eglContext;
	
	Osp::Graphics::Opengl::GLuint     __texture[2];
	unsigned int                      __texture_index;
	Osp::Graphics::Opengl::EGLSurface __pbuffer_surface;
	Osp::Graphics::Opengl::GLuint     __pbuffer_texture;
	int                               __pbuffer_width;
	int                               __pbuffer_height;
	Osp::Graphics::Opengl::EGLSurface __pixmap_surface;
	Osp::Graphics::Bitmap*            __pBitmap;

	Osp::Base::Runtime::Timer*        __pTimer;

	Osp::Ui::Controls::Form*          __pForm;
};

#endif
