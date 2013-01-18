package com.mapswithme.maps.pins;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.widget.ImageView;

public class CustomMapView extends ImageView
{

  private PinItem mNewPin;

  public CustomMapView(Context context)
  {
    super(context, null, 0);
  }

  public CustomMapView(Context context, AttributeSet attrs)
  {
    super(context, attrs, 0);
  }

  public CustomMapView(Context context, AttributeSet attrs, int defStyle)
  {
    super(context, attrs, defStyle);
  }

  void drawPin(int x, int y, Bitmap icon)
  {
    mNewPin = new PinItem(x, y, icon);
  }

  void dropPin()
  {
    mNewPin = null;
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    super.onDraw(canvas);
    if (mNewPin != null)
    {
      canvas.drawBitmap(mNewPin.icon, mNewPin.x - mNewPin.icon.getWidth() / 2, mNewPin.y, new Paint());
    }
  }

  private class PinItem
  {
    int x;
    int y;
    Bitmap icon;

    public PinItem(int x, int y, Bitmap icon)
    {
      super();
      this.x = x;
      this.y = y;
      this.icon = icon;
    }

  }
}
