package com.mapswithme.yopme.map;

import android.graphics.Bitmap;
import com.mapswithme.yopme.PoiPoint;
import com.mapswithme.yopme.map.MwmFilesObserver.EventType;
import com.mapswithme.yopme.map.MwmFilesObserver.MwmFilesListener;
import com.mapswithme.yopme.util.PixelBuffer;

public class MapRenderer implements MapDataProvider, MwmFilesListener
{
	private final static String TAG = "MapRenderer";
	PixelBuffer mPixelBuffer = null;

	private final MwmFilesObserver mFilesObserver;

	public MapRenderer(int width, int height)
	{
		mPixelBuffer = new PixelBuffer(width, height);
		mPixelBuffer.init();
		nativeCreateFramework(width, height);

		mFilesObserver = new MwmFilesObserver(this);
		mFilesObserver.startWatching();
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
  public MapData getMapData(PoiPoint viewPortCenter, double zoom, PoiPoint poi, PoiPoint myLocation)
  {
    synchronized (MapRenderer.class)
    {
      mPixelBuffer.attachToThread();

      final double vpLat = viewPortCenter.getLat();
      final double vpLon = viewPortCenter.getLon();

      final boolean hasPoi = poi != null;
      final double poiLat = hasPoi ? poi.getLat() : 0;
      final double poiLon = hasPoi ? poi.getLon() : 0;

      final boolean hasLocation = myLocation != null;
      final double myLat = hasLocation ? myLocation.getLat() : 0;
      final double myLon = hasLocation ? myLocation.getLon() : 0;

     if (nativeRenderMap(vpLat, vpLon, zoom,
                         hasPoi, poiLat, poiLon,
                         hasLocation, myLat, myLon))
     {
       final Bitmap bmp = mPixelBuffer.readBitmap();
       mPixelBuffer.detachFromThread();
       return new MapData(bmp, poi);
     }
     else
       return new MapData(null, poi);
    }
  }


	private native void    nativeCreateFramework (int width,  int height);

	private native boolean nativeRenderMap(double  vpLat,       double vpLon,  double zoom,
	                                       boolean hasPoi,      double poiLat, double poiLon,
	                                       boolean hasLocation, double myLat,  double myLon);

	private native void nativeOnKmlFileUpdate();
	private native void nativeOnMapFileUpdate();

  @Override
  public void onFileEvent(String path, EventType event)
  {
    synchronized (MapRenderer.class)
    {
      if (EventType.KML_FILE_EVENT  == event)
        nativeOnKmlFileUpdate();
      else if (EventType.MAP_FILE_EVENT == event)
        nativeOnMapFileUpdate();
    }
  }
}
