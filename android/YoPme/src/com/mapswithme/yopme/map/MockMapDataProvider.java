package com.mapswithme.yopme.map;

import android.content.Context;
import android.graphics.BitmapFactory;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.yopme.R;

public class MockMapDataProvider implements MapDataProvider
{
  private final Context mContext;

  public MockMapDataProvider(Context context)
  {
    mContext = context;
  }

  @Override
  public MapData getMyPositionData(double lat, double lon, double zoom)
  {
    final MWMPoint point = new MWMPoint(0, 0, "Minsk");
    return new MapData(BitmapFactory.decodeResource(mContext.getResources(), R.drawable.minsk_ink), point);
  }

  @Override
  public MapData getPOIData(MWMPoint poi, double zoom)
  {
    final MWMPoint point = new MWMPoint(0, 0, "Moskow");
    return new MapData(BitmapFactory.decodeResource(mContext.getResources(), R.drawable.msk_ink), point);
  }

}
