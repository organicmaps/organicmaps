package com.mapswithme.yopme.util;

import java.nio.IntBuffer;
import java.util.Arrays;
import java.util.Comparator;

import javax.microedition.khronos.egl.*;
import javax.microedition.khronos.opengles.GL11;

import android.graphics.Bitmap;
import android.opengl.GLES10;
import android.util.Log;

import static javax.microedition.khronos.egl.EGL11.*;

public class PixelBuffer
{
	private final String TAG = "PixelBuffer";
	//private static final int EGL_RENDERABLE_TYPE = 0x3040;
  	//private static final int EGL_OPENGL_ES2_BIT = 0x0004;
  	private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
  	
	private final EGL10 mEgl = (EGL10)EGLContext.getEGL();
	private EGLDisplay mDisplay;
	private EGLContext mContext;
	private EGLSurface mSurface;
	private GL11 mGL = null;
	
	private int mWidth;
	private int mHeight;
	
	public PixelBuffer(int width, int height)
	{
		mWidth = width;
		mHeight = height;
	}
	
	public void Init() throws EglInitializeException 
	{
		mDisplay = mEgl.eglGetDisplay(EGL11.EGL_DEFAULT_DISPLAY);
		if (mDisplay == EGL11.EGL_NO_DISPLAY)
			throw new EglInitializeException("EGL error : No display", mEgl.eglGetError());
		
		int[] version = new int[2];
		if (!mEgl.eglInitialize(mDisplay, version))
			throw new EglInitializeException("EGL error : initialize fault", mEgl.eglGetError());
		
		EGLConfig[] configs = GetConfigs();
		for (int i = 0; i < configs.length; ++i)
		{
			mSurface = CreateSurface(configs[i]);
			if (mSurface == EGL_NO_SURFACE)
				continue;
			
			mContext = CreateContext(configs[i]);
			if (mContext == EGL_NO_CONTEXT)
			{
				mEgl.eglDestroySurface(mDisplay, mSurface);
				continue;
			}
			
			break;
		}
		
		if (mSurface == EGL_NO_SURFACE)
			throw new EglInitializeException("EGL error : Surface not created", mEgl.eglGetError());
		
		if (mContext == EGL_NO_CONTEXT)
			throw new EglInitializeException("EGL error : Context not created", mEgl.eglGetError());
		
		Log.d(TAG, "Egl inited");
	}
	
	public void Terminate()
	{
		if (mDisplay != EGL_NO_DISPLAY)
		{
			mEgl.eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			if (mContext != EGL_NO_CONTEXT)
				mEgl.eglDestroyContext(mDisplay, mContext);
			if (mSurface != EGL_NO_SURFACE)
				mEgl.eglDestroySurface(mDisplay, mSurface);
			
			mEgl.eglTerminate(mDisplay);
		}
		
		mDisplay = EGL_NO_DISPLAY;
		mSurface = EGL_NO_SURFACE;
		mContext = EGL_NO_CONTEXT;
		Log.d(TAG, "Egl terminated");
	}
	
	public void AttachToThread()
	{
		Log.d(TAG, "Pixel buffer attached");
		if (!mEgl.eglMakeCurrent(mDisplay, mSurface, mSurface, mContext))
			throw new EglRuntimeException("EGL error : Context was not bind to thread", mEgl.eglGetError());
		
		mGL = (GL11)mContext.getGL();
	}
	
	public void DetachFromThread()
	{
		Log.d(TAG, "Pixel buffer detached");
		if (!mEgl.eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
			throw new EglRuntimeException("EGL error : Context was not bind to thread", mEgl.eglGetError());
		
		mGL = null;
	}
	
	public Bitmap ReadBitmap()
	{
		if (mEgl == null)
			return Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_4444);
		
		IntBuffer ib = IntBuffer.allocate(mWidth * mHeight);
        IntBuffer ibt = IntBuffer.allocate(mWidth * mHeight);
        mGL.glReadPixels(0, 0, mWidth, mHeight, GLES10.GL_RGBA, GLES10.GL_UNSIGNED_BYTE, ib);

        // Convert upside down mirror-reversed image to right-side up normal image.
        for (int i = 0; i < mHeight; i++) {     
            for (int j = 0; j < mWidth; j++) {
                ibt.put((mHeight-i-1)*mWidth + j, ib.get(i*mWidth + j));
            }
        }                  

        Bitmap bmp = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_8888);
        bmp.copyPixelsFromBuffer(ibt);
        return bmp;
	}
	
	class ConfigSorter implements Comparator<EGLConfig>
	{
		@Override
		public int compare(EGLConfig lhs, EGLConfig rhs)
		{
			return GetWeight(lhs) - GetWeight(rhs);
		}
		
		private int GetWeight(EGLConfig config)
		{
			int[] value = new int[1];
			mEgl.eglGetConfigAttrib(mDisplay, config, EGL_CONFIG_CAVEAT, value);
			
			switch (value[0]) 
			{
			case EGL_NONE: 					return 0;
			case EGL_SLOW_CONFIG:   		return 1;
			case EGL_NON_CONFORMANT_CONFIG: return 2;
			}
			
			return 0;
		}
	}
	
	private EGLContext CreateContext(EGLConfig config)
	{
		int[] contextAttributes = new int[] {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
		return mEgl.eglCreateContext(mDisplay, config, EGL_NO_CONTEXT, contextAttributes);
	}
	
	private EGLSurface CreateSurface(EGLConfig config)
	{
		int[] surfaceAttribs = new int[] { EGL_HEIGHT, mHeight,
										   EGL_WIDTH , mWidth,
										   EGL_NONE};
		
		return mEgl.eglCreatePbufferSurface(mDisplay, config, surfaceAttribs);
	}
	
	private EGLConfig[] GetConfigs() throws EglInitializeException
	{
		EGLConfig[] configs = new EGLConfig[40];
		int[] numConfigs = new int[] { 0 };
		
		int[] configAttributes = { EGL_RED_SIZE, 		EGL_DONT_CARE,
								   EGL_GREEN_SIZE, 		EGL_DONT_CARE,
								   EGL_BLUE_SIZE,		EGL_DONT_CARE,
								   //EGL_STENCIL_SIZE, 	0,
								   //EGL_DEPTH_SIZE,	   16,
								   //EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
								   EGL_NONE };
		
		if (!mEgl.eglChooseConfig(mDisplay, configAttributes, configs, 40, numConfigs))
			throw new EglInitializeException("EGL error : config not choosed", mEgl.eglGetError());
		
		if (numConfigs[0] == 0)
			throw new EglInitializeException("EGL error : config not founded", mEgl.eglGetError());
		
		Log.d(TAG, "Config numbers = " + numConfigs[0]);
		
		EGLConfig[] result = Arrays.copyOf(configs, numConfigs[0]);
		Arrays.sort(result, new ConfigSorter());
		
		return result;		
	}
}
