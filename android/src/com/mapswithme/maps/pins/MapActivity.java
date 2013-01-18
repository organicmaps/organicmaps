package com.mapswithme.maps.pins;

import com.mapswithme.maps.R;

import android.app.Activity;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.View.OnTouchListener;

public class MapActivity extends Activity
{
  private TouchDetector mDetector;
  private CustomMapView mMap;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_map);
    mDetector = new TouchDetector();
    (mMap = (CustomMapView) findViewById(R.id.map_map)).setOnLongClickListener(mDetector);
    findViewById(R.id.map_map).setOnTouchListener(mDetector);
    findViewById(R.id.map_map).setOnClickListener(mDetector);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    getMenuInflater().inflate(R.menu.activity_map, menu);
    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    int itemId = item.getItemId();
    if (itemId == R.id.menu_settings)
    {
      startActivity(new Intent(this, PinSetsActivity.class));
    }
    else
    {
    }
    return super.onOptionsItemSelected(item);
  }

  @Override
  protected void onDestroy()
  {
    mDetector.destroyPopup();
    super.onDestroy();
  }

  private class TouchDetector implements OnLongClickListener, OnTouchListener, OnClickListener
  {
    private int x;
    private int y;
    private PinPopup mPopup;

    private void destroyPopup()
    {
      if (mPopup != null)
      {
        mPopup.dismiss();
        mPopup = null;
        mMap.dropPin();
        mMap.invalidate();
      }
    }

    @Override
    public boolean onTouch(View v, MotionEvent event)
    {
      v.onTouchEvent(event);
      Log.d("Coord", "");
      if (event.getAction() == MotionEvent.ACTION_DOWN)
      {
        x = (int) event.getX();
        y = (int) event.getY();
      }
      return false;
    }

    @Override
    public boolean onLongClick(View v)
    {
      if (mPopup == null)
      {
        mPopup = new PinPopup(MapActivity.this);
        mPopup.getContentView().measure(v.getWidth(), v.getHeight());
        mPopup.showAtLocation(v, Gravity.NO_GRAVITY, x - mPopup.getContentView().getMeasuredWidth() / 2, y);
        mMap.drawPin(x, y, BitmapFactory.decodeResource(getResources(), R.drawable.pin_red));
        mMap.invalidate();
        mPopup.update();
      }
      return true;
    }

    @Override
    public void onClick(View v)
    {
      destroyPopup();
    }
  }
}
