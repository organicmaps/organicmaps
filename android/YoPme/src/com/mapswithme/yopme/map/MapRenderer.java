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
	    final MWMPoint poi = new MWMPoint(lat, lon, "");

  		mPixelBuffer.attachToThread();
  		if (nativeRenderMap(lat, lon, zoom, false, true, lat, lon) == false)
  		  return new MapData(null, poi);

  		Bitmap bmp = mPixelBuffer.readBitmap();
  		mPixelBuffer.detachFromThread();
  		return new MapData(bmp, poi);
  	}
	}

	@Override
	public MapData getPOIData(MWMPoint poi, double zoom, boolean myLocationDetected, double myLat, double myLon)
	{
	  synchronized(MapRenderer.class)
    {
  		mPixelBuffer.attachToThread();
  		if (nativeRenderMap(poi.getLat(), poi.getLon(), zoom, true, myLocationDetected, myLat, myLon) == false)
  		  return new MapData(null, poi);

  		Bitmap bmp = mPixelBuffer.readBitmap();
  		mPixelBuffer.detachFromThread();
  		return new MapData(bmp, poi);
    }
	}
	
	private native void nativeCreateFramework(int width, int height);
	private native boolean nativeRenderMap(double lat, double lon, double zoom,
	                                       boolean needApiMark, boolean needMyLocation,
	                                       double myLat, double myLon);
}
