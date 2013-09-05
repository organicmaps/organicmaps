package com.mapswithme.yopme.map;

import android.graphics.Bitmap;
import android.opengl.GLES10;
import android.util.Log;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.yopme.util.EglInitializeException;
import com.mapswithme.yopme.util.PixelBuffer;

public class MapRenderer implements MapDataProvider
{
	private final static String TAG = "MapRenderer";
	PixelBuffer mPixelBuffer = null;
	
	public MapRenderer(int width, int height) throws EglInitializeException
	{
		mPixelBuffer = new PixelBuffer(width, height);
		mPixelBuffer.Init();
	}
	
	public void Terminate()
	{
		mPixelBuffer.Terminate();
	}
	
	static volatile private MapRenderer mRenderer = null;
	
	static public MapRenderer GetRenderer() throws EglInitializeException
	{
		if (mRenderer == null)
			mRenderer = new MapRenderer(360, 540);
		return mRenderer;
	}

	@Override
	public MapData getMyPositionData(double lat, double lon, double zoom)
	{
		mPixelBuffer.AttachToThread();
		//nativeRenderMap();
		GLES10.glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
		GLES10.glClear(GLES10.GL_COLOR_BUFFER_BIT);
		Bitmap bmp = mPixelBuffer.ReadBitmap();
		mPixelBuffer.DetachFromThread();
		return new MapData(bmp, new MWMPoint(0.0, 0.0, "Clear Image"));
	}

	@Override
	public MapData getPOIData(MWMPoint poi, double zoom)
	{
		mPixelBuffer.AttachToThread();
		Log.i(TAG, "POI data call");
		//nativeRenderMap();
		GLES10.glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
		GLES10.glClear(GLES10.GL_COLOR_BUFFER_BIT);
		Bitmap bmp = mPixelBuffer.ReadBitmap();
		mPixelBuffer.DetachFromThread();
		return new MapData(bmp, poi);
	}
	
	//private native void nativeRenderMap();
}
