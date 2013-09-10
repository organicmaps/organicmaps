package com.mapswithme.yopme.map;

import android.graphics.Bitmap;
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
		nativeCreateFramework(width, height);
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
		}
		return mRenderer;
	}

	@Override
	public MapData getMyPositionData(double lat, double lon, double zoom)
	{
	  synchronized(MapRenderer.class)
	  {
  		mPixelBuffer.attachToThread();
  		nativeRenderMap(lat, lon, zoom);
  		Bitmap bmp = mPixelBuffer.readBitmap();
  		mPixelBuffer.detachFromThread();
  		return new MapData(bmp, new MWMPoint(lat, lon, ""));
  	}
	}

	@Override
	public MapData getPOIData(MWMPoint poi, double zoom)
	{
	  synchronized(MapRenderer.class)
    {
  		mPixelBuffer.attachToThread();
  		nativeRenderMap(poi.getLat(), poi.getLon(), zoom);
  		Bitmap bmp = mPixelBuffer.readBitmap();
  		mPixelBuffer.detachFromThread();
  		return new MapData(bmp, poi);
    }
	}
	
	private native void nativeCreateFramework(int width, int height);
	private native void nativeRenderMap(double lat, double lon, double zoom);
}
