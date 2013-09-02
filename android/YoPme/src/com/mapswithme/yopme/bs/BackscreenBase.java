package com.mapswithme.yopme.bs;

import com.mapswithme.yopme.R;
import com.mapswithme.yopme.map.MapData;
import com.mapswithme.yopme.map.MapDataProvider;
import com.mapswithme.yopme.map.MockMapDataProvider;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSDrawer;
import com.yotadevices.sdk.BSDrawer.Waveform;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

public abstract class BackscreenBase implements Backscreen
{
  protected View mView;
  protected ImageView mMapView;
  protected TextView mPoiText;

  protected BSActivity mBsActivity;
  protected int mResId;

  protected MapDataProvider mMapDataProvider;
  protected MapData mMapData;

  public BackscreenBase(BSActivity bsActivity)
  {
    mBsActivity = bsActivity;
    mResId = R.layout.yota_backscreen;
    //TODO: you know what to do with mocks
    mMapDataProvider = new MockMapDataProvider(bsActivity);
    setUpView();
  };

  public BackscreenBase(BSActivity bsActivity, int viewResId)
  {
    this(bsActivity);
    mResId = viewResId;
  };

  @Override
  public void onCreate()
  {
  }

  @Override
  public void onResume()
  {
  }

  @Override
  public void setUpView()
  {
    mView = View.inflate(mBsActivity, mResId, null);
    mMapView = (ImageView) mView.findViewById(R.id.map);
    mPoiText = (TextView) mView.findViewById(R.id.poiText);
  }

  protected void updateData(MapData mapData)
  {
    mMapView.setImageBitmap(mapData.getBitmap());
    mPoiText.setText(mapData.getPoint().getName());
  }

  @Override
  public void invalidate()
  {
    draw();
  }

  @Override
  public void draw(BSDrawer drawer)
  {
    if (mView != null)
      drawer.drawBitmap(mView, Waveform.WAVEFORM_GC_FULL);
  }

  protected void draw()
  {
    draw(mBsActivity.getBSDrawer());
  }

}
