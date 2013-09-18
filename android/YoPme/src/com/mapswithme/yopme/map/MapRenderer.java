package com.mapswithme.yopme.map;

import android.graphics.Bitmap;

import com.mapswithme.yopme.PoiPoint;
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
			mRenderer = new MapRenderer(360, 640);
		return mRenderer;
	}

	@Override
	public MapData getMyPositionData(double lat, double lon, double zoom)
	{
	  synchronized(MapRenderer.class)
	  {
	    final PoiPoint poi = new PoiPoint(lat, lon, "");

  		mPixelBuffer.attachToThread();
  		if (nativeRenderMyPosition(poi.getLat(), poi.getLon(), zoom) == false)
  		  return new MapData(null, poi);

		final Bitmap bmp = mPixelBuffer.readBitmap();
  		mPixelBuffer.detachFromThread();
  		return new MapData(bmp, poi);
  	}
	}

	@Override
	public MapData getPOIData(PoiPoint poi, double zoom, boolean myLocationDetected, double myLat, double myLon)
	{
	  synchronized(MapRenderer.class)
    {
  		mPixelBuffer.attachToThread();
  		if (nativeRenderPoiMap(poi.getLat(), poi.getLon(), myLocationDetected, myLat, myLon, zoom) == false)
  		  return new MapData(null, poi);

		final Bitmap bmp = mPixelBuffer.readBitmap();
  		mPixelBuffer.detachFromThread();
  		return new MapData(bmp, poi);
    }
	}

	private native void nativeCreateFramework(int width, int height);
	private native boolean nativeRenderMyPosition(double lat, double lon, double zoom);
	private native boolean nativeRenderPoiMap(double lat, double lon,
	                                              boolean needMyLocation, double myLat, double myLon,
	                                              double zoom);
}
