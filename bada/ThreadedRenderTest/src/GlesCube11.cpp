
#include "GlesCube11.h"
#include <math.h>

using namespace Osp::Base;
using namespace Osp::Base::Runtime;
using namespace Osp::Graphics;
using namespace Osp::Locales;
using namespace Osp::System;
using namespace Osp::App;
using namespace Osp::System;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::Graphics::Opengl;

#define THREADED

#define TIME_OUT  10

#define USE_PBUFFER

using namespace Osp::Graphics::Opengl;

class GlesForm :
	public Osp::Ui::Controls::Form
{
public:
	GlesForm(GlesCube11* pApp)
	: __pApp(pApp)
	{
	}
	virtual ~GlesForm(void)
	{
	}

public:
	virtual result OnDraw(void)
	{
		if (__pApp)
		{
			__pApp->Draw();
		}
		return E_SUCCESS;
	}

private:
	GlesCube11* __pApp;
};


namespace
{
	int GetPowerOf2(int value)
	{
		int result = 1;

		while (result < value)
			result <<= 1;

		return result;
	}


	typedef float GlUnit;

	GlUnit GetGlUnit(float vaule)
	{
		return vaule;
	}
	GlUnit GetGlUnit(int val)
	{
		return float(val);
	}

	#define glClearColorEx glClearColor
	#define glFrustum      glFrustumf
	#define glFog          glFogf
	#define glTranslate    glTranslatef
	#define glRotate       glRotatef
	#define GL_TFIXED      GL_FLOAT

	const GlUnit ONEP = GetGlUnit(+1.0f);
	const GlUnit ONEN = GetGlUnit(-1.0f);
	const GlUnit ZERO = GetGlUnit( 0.0f);


	void SetPerspective(GLfloat fovDegree, GLfloat aspect, GLfloat zNear,  GLfloat zFar)
	{
		// tan(double(degree) * 3.1415962 / 180.0 / 2.0);
		static const float HALF_TAN_TABLE[91] =
		{
			0.00000f, 0.00873f, 0.01746f, 0.02619f, 0.03492f, 0.04366f, 0.05241f, 0.06116f, 0.06993f,
			0.07870f, 0.08749f, 0.09629f, 0.10510f, 0.11394f, 0.12278f, 0.13165f, 0.14054f, 0.14945f,
			0.15838f, 0.16734f, 0.17633f, 0.18534f, 0.19438f, 0.20345f, 0.21256f, 0.22169f, 0.23087f,
			0.24008f, 0.24933f, 0.25862f, 0.26795f, 0.27732f, 0.28675f, 0.29621f, 0.30573f, 0.31530f,
			0.32492f, 0.33460f, 0.34433f, 0.35412f, 0.36397f, 0.37389f, 0.38386f, 0.39391f, 0.40403f,
			0.41421f, 0.42448f, 0.43481f, 0.44523f, 0.45573f, 0.46631f, 0.47698f, 0.48773f, 0.49858f,
			0.50953f, 0.52057f, 0.53171f, 0.54296f, 0.55431f, 0.56577f, 0.57735f, 0.58905f, 0.60086f,
			0.61280f, 0.62487f, 0.63707f, 0.64941f, 0.66189f, 0.67451f, 0.68728f, 0.70021f, 0.71329f,
			0.72654f, 0.73996f, 0.75356f, 0.76733f, 0.78129f, 0.79544f, 0.80979f, 0.82434f, 0.83910f,
			0.85408f, 0.86929f, 0.88473f, 0.90041f, 0.91633f, 0.93252f, 0.94897f, 0.96569f, 0.98270f,
			1.00000f
		};

		int degree = int(fovDegree + 0.5f);

		degree = (degree >=  0) ? degree :  0;
		degree = (degree <= 90) ? degree : 90;

		GlUnit fxdYMax  = GetGlUnit(zNear * HALF_TAN_TABLE[degree]);
		GlUnit fxdYMin  = -fxdYMax;

		GlUnit fxdXMax  = GetGlUnit(GLfloat(fxdYMax) * aspect);

		GlUnit fxdXMin  = -fxdXMax;

		glFrustum(fxdXMin, fxdXMax, fxdYMin, fxdYMax, GetGlUnit(zNear), GetGlUnit(zFar));
	}

	class RenderToTexture
	{
	public:
		RenderToTexture(EGLDisplay eglDisplay, EGLSurface pbuffer_surface, GLuint texture)
		: __eglDisplay(eglDisplay), __pbuffer_surface(pbuffer_surface), __texture(texture)
		{
			if (__pbuffer_surface)
			{
				__eglSurfaceR = eglGetCurrentSurface(EGL_READ);
				__eglSurfaceW = eglGetCurrentSurface(EGL_DRAW);
				__eglContext  = eglGetCurrentContext();

				eglMakeCurrent(__eglDisplay, __pbuffer_surface, __pbuffer_surface, __eglContext);
				glBindTexture(GL_TEXTURE_2D, 0);
				eglReleaseTexImage(__eglDisplay, __pbuffer_surface, EGL_BACK_BUFFER);
			}
		}
		~RenderToTexture()
		{
			if (__pbuffer_surface)
			{
				glBindTexture(GL_TEXTURE_2D, __texture);
				eglBindTexImage(__eglDisplay, __pbuffer_surface, EGL_BACK_BUFFER);
				eglMakeCurrent(__eglDisplay, __eglSurfaceW, __eglSurfaceR, __eglContext);
			}
		}
		
	private:
		EGLDisplay	__eglDisplay;
		EGLSurface	__eglSurfaceR;
		EGLSurface	__eglSurfaceW;
		EGLContext	__eglContext;
		EGLSurface  __pbuffer_surface;
		GLuint      __texture;
	};

}

#define RED_SIZE 8
#define GREEN_SIZE 8
#define BLUE_SIZE 8
#define ALPHA_SIZE 8
#define DEPTH_SIZE 16
#define ITERCOUNT 100

extern const unsigned short image565_128_128_1[];
extern const unsigned short image4444_128_128_1[];

class ThreadRenderer : public Osp::Base::Runtime::Thread
{
public:
	GlesCube11 * m_app;
	EGLDisplay m_display;
	EGLConfig  m_config;
	EGLContext m_context;
	EGLSurface m_pbuffer_surface;
	GLuint m_pbuffer_texture;
	int m_pbuffer_width;
	int m_pbuffer_height;
	int m_width;
	int m_height;
	GLuint m_textures[2];
	bool m_isProcessing;

	ThreadRenderer(
			GlesCube11 * app,
			EGLDisplay display,
			EGLConfig  config,
			EGLContext context,
			int width,
			int height
			)
		: m_app(app),
		  m_display(display),
		  m_config(config),
		  m_width(width),
		  m_height(height),
		  m_isProcessing(true)
	{
		EGLint numConfigs = 1;
		EGLint eglConfigList[] = {
			EGL_RED_SIZE,	RED_SIZE,
			EGL_GREEN_SIZE,	GREEN_SIZE,
			EGL_BLUE_SIZE,	BLUE_SIZE,
			EGL_ALPHA_SIZE,	ALPHA_SIZE,
			EGL_DEPTH_SIZE, DEPTH_SIZE,
			EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE
		};

		eglChooseConfig(m_display, eglConfigList, &m_config, 1, &numConfigs);

		EGLint surfaceType;

		eglGetConfigAttrib(m_display, m_config, EGL_SURFACE_TYPE, &surfaceType);

		if ((surfaceType & EGL_PBUFFER_BIT) > 0)
		{
			EGLint pbuffer_attribs[] =
			{
				EGL_WIDTH,                  GetPowerOf2(m_width),
				EGL_HEIGHT,                 GetPowerOf2(m_height),
				EGL_TEXTURE_TARGET,         EGL_TEXTURE_2D,
				EGL_TEXTURE_FORMAT,         EGL_TEXTURE_RGB,
				EGL_NONE
			};

			m_pbuffer_surface = eglCreatePbufferSurface(m_display, m_config, pbuffer_attribs);

			eglQuerySurface(m_display, m_pbuffer_surface, EGL_WIDTH, &m_pbuffer_width);
			eglQuerySurface(m_display, m_pbuffer_surface, EGL_HEIGHT, &m_pbuffer_height);

			glGenTextures(1, &m_pbuffer_texture);
			glBindTexture(GL_TEXTURE_2D, m_pbuffer_texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

	    EGLint eglContextList[] =
	    { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE };

		m_context = eglCreateContext(m_display, m_config, context, eglContextList);
	}

	void InitGL()
	{

		{
			glGenTextures(2, m_textures);

			glBindTexture(GL_TEXTURE_2D, m_textures[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, image4444_128_128_1);
			glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glBindTexture(GL_TEXTURE_2D, m_textures[1]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, image565_128_128_1);
			glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glShadeModel(GL_SMOOTH);

		glViewport(0, 0, m_width, m_height);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		{
			glEnable(GL_FOG);
			glFogx(GL_FOG_MODE,   GL_LINEAR);
			glFog(GL_FOG_DENSITY, GetGlUnit(0.25f));
			glFog(GL_FOG_START,   GetGlUnit(4.0f));
			glFog(GL_FOG_END,     GetGlUnit(6.5f));
			glHint(GL_FOG_HINT,   GL_DONT_CARE);
		}

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		SetPerspective(60.0f, 1.0f * m_width / m_height, 1.0f, 400.0f);

		glClearColorEx(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void DrawLoop()
	{
		m_app->m_primaryMonitor->Enter();
		int count = 0;
		while (count++ < ITERCOUNT)
		{
			struct
			{
				GLint x, y, width, height;
			} viewPort;

			glGetIntegerv(GL_VIEWPORT, (GLint*)&viewPort);

			{
				const  double PI  = 3.141592;
				static double hue = 0.0;

				float r = (1.0f + float(sin(hue - 2.0 * PI / 3.0))) / 3.0f;
				float g = (1.0f + float(sin(hue)                 )) / 3.0f;
				float b = (1.0f + float(sin(hue + 2.0 * PI / 3.0))) / 3.0f;

				GLfloat fogColor[4] =
				{
					r, g, b, 1.0f
				};

				glFogfv(GL_FOG_COLOR, fogColor);

				glClearColorEx(GetGlUnit(r), GetGlUnit(g), GetGlUnit(b), GetGlUnit(1.0f));

				hue += 0.03;
			}

			{
				RenderToTexture fbo(m_display, m_pbuffer_surface, m_pbuffer_texture);

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				m_app->DrawCube1();
				m_app->DrawCube2();

				glFlush();
				glFinish();

			}

			m_app->m_primaryMonitor->Notify();
			/// Wait for primary thread to process the rendered target
			m_app->m_primaryMonitor->Wait();

		}

		m_app->m_primaryMonitor->Exit();
		m_isProcessing = false;
	}

	Osp::Base::Object * Run()
	{
		eglMakeCurrent(m_display, m_pbuffer_surface, m_pbuffer_surface, m_context);
		InitGL();
		DrawLoop();
		return 0;
	}

};

GlesCube11::GlesCube11()
:	__eglDisplay(EGL_DEFAULT_DISPLAY),
	__eglSurface(EGL_NO_SURFACE),
	__eglConfig(null),
	__eglContext(EGL_NO_CONTEXT),
	__texture_index(0),
	__pbuffer_surface(EGL_NO_SURFACE),
	__pbuffer_texture(0),
	__pbuffer_width(0),
	__pbuffer_height(0),
	__pixmap_surface(EGL_NO_SURFACE),
	__pTimer(null),
	__pForm(null)
{
	__texture[0] = 0;
	__texture[1] = 0;
}


GlesCube11::~GlesCube11()
{
}

void
GlesCube11::Cleanup()
{
	if (__pTimer)
	{
		__pTimer->Cancel();
		delete __pTimer;
		__pTimer = null;
	}

	DestroyGL();
}


Application*
GlesCube11::CreateInstance(void)
{
	return new GlesCube11();
}


bool
GlesCube11::OnAppInitializing(AppRegistry& appRegistry)
{
	Timer* tempTimer = null;
	result r = E_SUCCESS;

	__pForm = new GlesForm(this);

	r = __pForm->Construct(FORM_STYLE_NORMAL);

	r = GetAppFrame()->GetFrame()->AddControl(*__pForm);

	InitEGL();

	InitGL();

	tempTimer = new Timer;

	r = tempTimer->Construct(*this);

	__pTimer  = tempTimer;
	tempTimer = 0;

	return true;
}


bool
GlesCube11::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination)
{
	Cleanup();

	return true;
}


void
GlesCube11::OnForeground(void)
{
	if (__pTimer)
	{
		__pTimer->Start(TIME_OUT);
	}
}


void
GlesCube11::OnBackground(void)
{
	if (__pTimer)
	{
		__pTimer->Cancel();
	}
}


void
GlesCube11::OnLowMemory(void)
{
}


void
GlesCube11::OnBatteryLevelChanged(BatteryLevel batteryLevel)
{
}


void
GlesCube11::OnTimerExpired(Timer& timer)
{
	if (!__pTimer)
	{
		return;
	}

	__pTimer->Start(TIME_OUT);

#ifdef THREADED
	if (m_renderer->m_isProcessing)
	{
		m_primaryMonitor->Notify();
		Draw();
	}
	else
	{
		if (m_primaryMonitor)
		{
			m_primaryMonitor->Exit();
			delete m_primaryMonitor;
			m_primaryMonitor = 0;
		}
	}
#else
	Draw();
#endif
}


bool
GlesCube11::InitEGL()
{
	EGLint numConfigs = 1;
	EGLint eglConfigList[] = {
		EGL_RED_SIZE,	RED_SIZE,
		EGL_GREEN_SIZE,	GREEN_SIZE,
		EGL_BLUE_SIZE,	BLUE_SIZE,
		EGL_ALPHA_SIZE,	ALPHA_SIZE,
		EGL_DEPTH_SIZE, DEPTH_SIZE,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLint eglContextList[] = {
		EGL_CONTEXT_CLIENT_VERSION, 1, 
		EGL_NONE
	};

	eglBindAPI(EGL_OPENGL_ES_API);

	if (__eglDisplay)
	{
		DestroyGL();
	}

	__eglDisplay = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
	if (EGL_NO_DISPLAY == __eglDisplay)
	{
		AppLog("[GlesCube11] eglGetDisplay() is failed. [0x%x]\n", eglGetError());
//		goto CATCH;
	}

	if (EGL_FALSE == eglInitialize(__eglDisplay, null, null) ||
		EGL_SUCCESS != eglGetError())
	{
		AppLog("[GlesCube11] eglInitialize() is failed. [0x%x]\n", eglGetError());
//		goto CATCH;
	}

	if (EGL_FALSE == eglChooseConfig(__eglDisplay, eglConfigList, &__eglConfig, 1, &numConfigs) ||
		EGL_SUCCESS != eglGetError())
	{
		AppLog("[GlesCube11] eglChooseConfig() is failed. [0x%x]\n", eglGetError());
//		goto CATCH;
	}

	if (!numConfigs)
	{
		AppLog("[GlesCube11] eglChooseConfig() has been failed. because of matching config doesn't exist \n");
//		goto CATCH;
	}

	__eglSurface = eglCreateWindowSurface(__eglDisplay, __eglConfig, (EGLNativeWindowType)__pForm, null);

	if (EGL_NO_SURFACE == __eglSurface ||
		EGL_SUCCESS != eglGetError())
	{
		AppLog("[GlesCube11] eglCreateWindowSurface() has been failed. EGL_NO_SURFACE [0x%x]\n", eglGetError());
//		goto CATCH;
	}

	__eglContext = eglCreateContext(__eglDisplay, __eglConfig, EGL_NO_CONTEXT, eglContextList);
	if (EGL_NO_CONTEXT == __eglContext ||
		EGL_SUCCESS != eglGetError())
	{
		AppLog("[GlesCube11] eglCreateContext() has been failed. [0x%x]\n", eglGetError());
//		goto CATCH;
	}

	if (false == eglMakeCurrent(__eglDisplay, __eglSurface, __eglSurface, __eglContext) ||
		EGL_SUCCESS != eglGetError())
	{
		AppLog("[GlesCube11] eglMakeCurrent() has been failed. [0x%x]\n", eglGetError());
//		goto CATCH;
	}


	int x, y, width, height;
	GetAppFrame()->GetFrame()->GetBounds(x, y, width, height);

	m_pbufferMonitor = new Osp::Base::Runtime::Monitor();

#ifdef THREADED

	m_primaryMonitor = new Osp::Base::Runtime::Monitor();
	m_primaryMonitor->Construct();
	m_primaryMonitor->Enter();

	ThreadRenderer * thread = new ThreadRenderer(this, __eglDisplay, __eglConfig, __eglContext, width, height);
	m_renderer = thread;
	thread->Construct();
	thread->Start();
#endif

	return true;

CATCH:
	{
		AppLog("[GlesCube11] GlesCube11 can run on systems which supports OpenGL ES(R) 1.1.");
		AppLog("[GlesCube11] Please check that your system supports OpenGL(R) 1.5 or later, and make sure that you have the latest graphics driver installed.");
	}

	DestroyGL();
	return false;
}

bool
GlesCube11::InitGL()
{
	int x, y, width, height;

	GetAppFrame()->GetFrame()->GetBounds(x, y, width, height);

	EGLint surfaceType;

	eglGetConfigAttrib(__eglDisplay, __eglConfig, EGL_SURFACE_TYPE, &surfaceType);

	if ((surfaceType & EGL_PBUFFER_BIT) > 0)
	{
		EGLint pbuffer_attribs[] =
		{
			EGL_WIDTH,                  GetPowerOf2(width),
			EGL_HEIGHT,                 GetPowerOf2(height),
			EGL_TEXTURE_TARGET,         EGL_TEXTURE_2D,
			EGL_TEXTURE_FORMAT,         EGL_TEXTURE_RGB,
			EGL_NONE
		};

		__pbuffer_surface = eglCreatePbufferSurface(__eglDisplay, __eglConfig, pbuffer_attribs);

		eglQuerySurface(__eglDisplay, __pbuffer_surface, EGL_WIDTH, &__pbuffer_width);
		eglQuerySurface(__eglDisplay, __pbuffer_surface, EGL_HEIGHT, &__pbuffer_height);

		glGenTextures(1, &__pbuffer_texture);
		glBindTexture(GL_TEXTURE_2D, __pbuffer_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	{
		glGenTextures(2, __texture);

		glBindTexture(GL_TEXTURE_2D, __texture[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, image4444_128_128_1);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_2D, __texture[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, image565_128_128_1);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glShadeModel(GL_SMOOTH);

	glViewport(0, 0, width, height);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	{
		glEnable(GL_FOG);
		glFogx(GL_FOG_MODE,   GL_LINEAR);
		glFog(GL_FOG_DENSITY, GetGlUnit(0.25f));
		glFog(GL_FOG_START,   GetGlUnit(4.0f));
		glFog(GL_FOG_END,     GetGlUnit(6.5f));
		glHint(GL_FOG_HINT,   GL_DONT_CARE);
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	SetPerspective(60.0f, 1.0f * width / height, 1.0f, 400.0f);

	glClearColorEx(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);


	return true;
}


void
GlesCube11::DestroyGL()
{
	glDeleteTextures(2, __texture);

	if (__pbuffer_texture)
	{
		glDeleteTextures(1, &__pbuffer_texture);
		__pbuffer_texture = 0;
	}

	if (EGL_NO_DISPLAY != __eglDisplay)
	{
		eglMakeCurrent(__eglDisplay, null, null, null);

		if (__pbuffer_surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(__eglDisplay, __pbuffer_surface);
			__pbuffer_surface = EGL_NO_SURFACE;
		}

		if (__pixmap_surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(__eglDisplay, __pixmap_surface);
			__pixmap_surface = EGL_NO_SURFACE;
		}

		if (__eglContext != EGL_NO_CONTEXT)
		{
			eglDestroyContext(__eglDisplay, __eglContext);
			__eglContext = EGL_NO_CONTEXT;
		}

		if (__eglSurface != EGL_NO_SURFACE)
		{
			eglDestroySurface(__eglDisplay, __eglSurface);
			__eglSurface = EGL_NO_SURFACE;
		}

		eglTerminate(__eglDisplay);
		__eglDisplay = EGL_NO_DISPLAY;
	}
	
	__eglConfig     = null;
	__texture_index = 0;
	__texture[0]    = 0;
	__texture[1]    = 0;
	
	return;
}

bool
GlesCube11::Draw()
{
	eglMakeCurrent(__eglDisplay, __eglSurface, __eglSurface, __eglContext);

	struct
	{
		GLint x, y, width, height;
	} viewPort;

	glGetIntegerv(GL_VIEWPORT, (GLint*)&viewPort);

	int width  = viewPort.width / 5;
	int height = width * viewPort.height / viewPort.width;

#ifndef THREADED

	{
		const  double PI  = 3.141592;
		static double hue = 0.0;

		float r = (1.0f + float(sin(hue - 2.0 * PI / 3.0))) / 3.0f;
		float g = (1.0f + float(sin(hue)                 )) / 3.0f;
		float b = (1.0f + float(sin(hue + 2.0 * PI / 3.0))) / 3.0f;

		GLfloat fogColor[4] =
		{
			r, g, b, 1.0f
		};

		glFogfv(GL_FOG_COLOR, fogColor);

		glClearColorEx(GetGlUnit(r), GetGlUnit(g), GetGlUnit(b), GetGlUnit(1.0f));

		hue += 0.03;
	}

	{
		RenderToTexture fbo(__eglDisplay, __pbuffer_surface, __pbuffer_texture);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		this->DrawCube1();
		this->DrawCube2();

		glFlush();
		glFinish();
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	this->DrawTexture2D( 10, 10, width, height, __pbuffer_texture, __pbuffer_width, __pbuffer_height);
	this->DrawTexture2D((viewPort.width-width-10), 10, width, height, __pbuffer_texture, __pbuffer_width, __pbuffer_height);
	this->DrawTexture2D(0, 0, viewPort.width, viewPort.height, __pbuffer_texture, __pbuffer_width, __pbuffer_height);


#else
	/// Wait for single frame from thread
	m_primaryMonitor->Wait();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	this->DrawTexture2D( 10, 10, width, height, m_renderer->m_pbuffer_texture, __pbuffer_width, __pbuffer_height);
	this->DrawTexture2D((viewPort.width-width-10), 10, width, height, m_renderer->m_pbuffer_texture, __pbuffer_width, __pbuffer_height);
	this->DrawTexture2D(0, 0, viewPort.width, viewPort.height, m_renderer->m_pbuffer_texture, __pbuffer_width, __pbuffer_height);

#endif

	eglSwapBuffers(__eglDisplay, __eglSurface);

	return true;
}

void
GlesCube11::DrawCube1(void)
{
	static const GlUnit vertices[] =
	{
		ONEN, ONEP, ONEN, // 0
		ONEP, ONEP, ONEN, // 1
		ONEN, ONEN, ONEN, // 2
		ONEP, ONEN, ONEN, // 3
		ONEN, ONEP, ONEP, // 4
		ONEP, ONEP, ONEP, // 5
		ONEN, ONEN, ONEP, // 6
		ONEP, ONEN, ONEP  // 7
	};

	static const GlUnit vertexColor[] =
	{
		ONEP, ZERO, ONEP, ONEP,
		ONEP, ONEP, ZERO, ONEP,
		ZERO, ONEP, ONEP, ONEP,
		ONEP, ZERO, ZERO, ONEP,
		ZERO, ZERO, ONEP, ONEP,
		ZERO, ONEP, ZERO, ONEP,
		ONEP, ONEP, ONEP, ONEP,
		ZERO, ZERO, ZERO, ONEP
	};

	static const unsigned short indexBuffer[] =
	{
		0, 1, 2, 2, 1, 3,
		1, 5, 3, 3, 5, 7,
		5, 4, 7, 7, 4, 6,
		4, 0, 6, 6, 0, 2,
		4, 5, 0, 0, 5, 1,
		2, 3, 6, 6, 3, 7
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_TFIXED, 0, vertices);

	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_TFIXED, 0, vertexColor);

	glMatrixMode(GL_MODELVIEW);
	{
		glLoadIdentity();
		glTranslate(0, GetGlUnit(-0.7f), GetGlUnit(-5.0f));
		{
			static int angle = 0;
			angle = (angle+1) % (360*3);
			glRotate(GetGlUnit(angle)/3, GetGlUnit(1.0f), 0, 0);
			glRotate(GetGlUnit(angle), 0, 0, GetGlUnit(1.0f));
		}
	}

	glDrawElements(GL_TRIANGLES, 6*(3*2), GL_UNSIGNED_SHORT, &indexBuffer[0]);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void
GlesCube11::DrawCube2(void)
{
	static const GlUnit vertices[] =
	{
		ONEN, ONEN, ONEP, ONEP, ONEN, ONEP, ONEN, ONEP, ONEP, ONEP, ONEP, ONEP,
		ONEN, ONEN, ONEN, ONEN, ONEP, ONEN, ONEP, ONEN, ONEN, ONEP, ONEP, ONEN,
		ONEN, ONEN, ONEP, ONEN, ONEP, ONEP, ONEN, ONEN, ONEN, ONEN, ONEP, ONEN,
		ONEP, ONEN, ONEN, ONEP, ONEP, ONEN, ONEP, ONEN, ONEP, ONEP, ONEP, ONEP,
		ONEN, ONEP, ONEP, ONEP, ONEP, ONEP, ONEN, ONEP, ONEN, ONEP, ONEP, ONEN,
		ONEN, ONEN, ONEP, ONEN, ONEN, ONEN, ONEP, ONEN, ONEP, ONEP, ONEN, ONEN
	};

	static const GlUnit textureCoord[] =
	{
		ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
		ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
		ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
		ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
		ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
		ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_TFIXED, 0, vertices);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_TFIXED, 0, textureCoord);

	glEnable(GL_TEXTURE_2D);
#ifdef THREADED
	glBindTexture(GL_TEXTURE_2D, m_renderer->m_textures[__texture_index]);
#else
	glBindTexture(GL_TEXTURE_2D, __texture[__texture_index]);
#endif
	glMatrixMode(GL_MODELVIEW);
	{
		const  float Z_POS_INC = 0.01f;
		static float zPos      = -5.0f;
		static float zPosInc   = Z_POS_INC;

		zPos += zPosInc;
		if (zPos < -8.0f)
		{
			zPosInc = Z_POS_INC;
			__texture_index = 1 - __texture_index;
		}
		if (zPos > -5.0f)
		{
			zPosInc = -Z_POS_INC;
		}

		glLoadIdentity();
		glTranslate(0, GetGlUnit(1.2f), GetGlUnit(zPos));
		{
			static int angle = 0;
			angle = (angle+1) % (360*3);
			glRotate(GetGlUnit(angle)/3, 0, 0, GetGlUnit(1.0f));
			glRotate(GetGlUnit(angle), 0, GetGlUnit(1.0f), 0);
		}
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4*6);

	glDisable(GL_TEXTURE_2D);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void
GlesCube11::DrawTexture2D(int x, int y, int w, int h, GLuint texture, int texWidth, int texHeight)
{
	const GLfixed Z_DEPTH = 50;

	struct
	{
		GLint x, y, width, height;
	} viewPort;

	glGetIntegerv(GL_VIEWPORT, (GLint*)&viewPort);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glOrthox(viewPort.x, viewPort.width, viewPort.y , viewPort.height, -Z_DEPTH, Z_DEPTH);

	const GLfixed x1 = x;
	const GLfixed y1 = viewPort.height - h - y;
	const GLfixed x2 = x1 + w;
	const GLfixed y2 = y1 + h;

	const GLfixed vertices[] =
	{
		x1, y1,
		x2, y1,
		x1, y2,
		x2, y2
	};

	const GLfixed texX1 = 0;
	const GLfixed texY1 = 0;
	const GLfixed texX2 = 65536 * viewPort.width  / texWidth;
	const GLfixed texY2 = 65536 * viewPort.height / texHeight;

	const GLfixed textureCoord[] =
	{
		texX1, texY1,
		texX2, texY1,
		texX1, texY2,
		texX2, texY2
	};

	static const unsigned short indexBuffer[] =
	{
		0, 1, 2, 3
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FIXED, 0, vertices);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FIXED, 0, textureCoord);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatex(0, 0, -Z_DEPTH+1);

	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, &indexBuffer[0]);

	glDisable(GL_TEXTURE_2D);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void
GlesCube11::OnScreenOn (void)
{
}

void
GlesCube11::OnScreenOff (void)
{
}


