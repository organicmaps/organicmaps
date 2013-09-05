package com.mapswithme.yopme.map;

import android.graphics.Bitmap;
import android.opengl.GLES10;
import android.util.Log;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.yopme.util.PixelBuffer;

public class MapRenderer implements MapDataProvider
{
	private final static String TAG = "MapRenderer";
	PixelBuffer mPixelBuffer = null;
	
	public MapRenderer(int width, int height)
	{
		mPixelBuffer = new PixelBuffer(width, height);
		mPixelBuffer.init();
	}
	
	public void terminate()
	{
		mPixelBuffer.terminate();
	}
	
	static volatile private MapRenderer mRenderer = null;
	
	static public MapRenderer GetRenderer()
	{
		if (mRenderer == null)
		{
			mRenderer = new MapRenderer(360, 640);
			Log.d(TAG, "Renderer created");
		}
		return mRenderer;
	}

	@Override
	public MapData getMyPositionData(double lat, double lon, double zoom)
	{
		mPixelBuffer.attachToThread();
		//nativeRenderMap();
		GLES10.glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
		GLES10.glClear(GLES10.GL_COLOR_BUFFER_BIT);
		Bitmap bmp = mPixelBuffer.readBitmap();
		mPixelBuffer.detachFromThread();
		Log.d(TAG, "Bitmap created");
		return new MapData(bmp, new MWMPoint(0.0, 0.0, "Clear Image"));
	}

	@Override
	public MapData getPOIData(MWMPoint poi, double zoom)
	{
		mPixelBuffer.attachToThread();
		//nativeRenderMap();
		GLES10.glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
		GLES10.glClear(GLES10.GL_COLOR_BUFFER_BIT);
		Bitmap bmp = mPixelBuffer.readBitmap();
		mPixelBuffer.detachFromThread();
		Log.d(TAG, "Bitmap created");
		return new MapData(bmp, poi);
	}
	
	//private native void nativeRenderMap();
}
