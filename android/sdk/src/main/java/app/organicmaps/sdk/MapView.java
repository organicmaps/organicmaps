package app.organicmaps.sdk;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import androidx.annotation.NonNull;
import androidx.core.content.res.ConfigurationHelper;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.sdk.util.log.Logger;

public class MapView extends SurfaceView
{
  private static final String TAG = MapView.class.getSimpleName();

  private class SurfaceHolderCallback implements SurfaceHolder.Callback
  {
    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder)
    {
      Logger.d(TAG);
      mMap.onSurfaceCreated(MapView.this.getContext(), holder.getSurface(), holder.getSurfaceFrame(),
                            ConfigurationHelper.getDensityDpi(MapView.this.getResources()));
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height)
    {
      Logger.d(TAG);
      mMap.onSurfaceChanged(MapView.this.getContext(), holder.getSurface(), holder.getSurfaceFrame(),
                            holder.isCreating());
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder)
    {
      Logger.d(TAG);
      mMap.onSurfaceDestroyed(isHostActivityChangingConfigurations());
    }
  }

  @NonNull
  private final Map mMap;

  public MapView(Context context)
  {
    this(context, null, 0);
  }

  public MapView(Context context, DisplayType displayType)
  {
    this(context, null, 0, 0, displayType);
  }

  public MapView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public MapView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    this(context, attrs, defStyleAttr, 0);
  }

  public MapView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    this(context, attrs, defStyleAttr, defStyleRes, DisplayType.Device);
  }

  private MapView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes,
                  @NonNull DisplayType displayType)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    mMap = new Map(displayType);
    getHolder().addCallback(new SurfaceHolderCallback());
  }

  public final void onDraw(@NonNull Canvas canvas)
  {
    super.onDraw(canvas);
    if (isInEditMode())
      drawMapPreview(canvas);
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    int action = event.getActionMasked();
    int pointerIndex = event.getActionIndex();
    switch (action)
    {
    case MotionEvent.ACTION_POINTER_UP -> action = Map.NATIVE_ACTION_UP;
    case MotionEvent.ACTION_UP ->
    {
      action = Map.NATIVE_ACTION_UP;
      pointerIndex = 0;
    }
    case MotionEvent.ACTION_POINTER_DOWN -> action = Map.NATIVE_ACTION_DOWN;
    case MotionEvent.ACTION_DOWN ->
    {
      action = Map.NATIVE_ACTION_DOWN;
      pointerIndex = 0;
    }
    case MotionEvent.ACTION_MOVE ->
    {
      action = Map.NATIVE_ACTION_MOVE;
      pointerIndex = Map.INVALID_POINTER_MASK;
    }
    case MotionEvent.ACTION_CANCEL -> action = Map.NATIVE_ACTION_CANCEL;
    }
    Map.onTouch(action, event, pointerIndex);
    performClick();
    return true;
  }

  @Override
  public boolean performClick()
  {
    super.performClick();
    return false;
  }

  @NonNull
  Map getMap()
  {
    return mMap;
  }

  ///  The function is called only in the design mode of Android Studio.
  private void drawMapPreview(@NonNull Canvas canvas)
  {
    final int w = getWidth();
    final int h = getHeight();

    // Background
    Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    paint.setStyle(Paint.Style.FILL);
    if (Utils.isDarkMode(getContext()))
      paint.setColor(Color.rgb(30, 30, 30));
    else
      paint.setColor(Color.rgb(245, 242, 230));
    canvas.drawRect(0, 0, w, h, paint);

    // Grid lines (lat/lon)
    paint.setColor(Color.LTGRAY);
    paint.setStrokeWidth(2f);
    final int step = Math.min(w, h) / 6;
    for (int i = 0; i < Math.max(w, h); i += step)
    {
      if (i < w)
        canvas.drawLine(i, 0, i, h, paint);
      if (i < h)
        canvas.drawLine(0, i, w, i, paint);
    }
  }

  private boolean isHostActivityChangingConfigurations()
  {
    Activity activity = findActivity(getContext());
    return activity != null && activity.isChangingConfigurations();
  }

  private static Activity findActivity(Context context)
  {
    while (context instanceof ContextWrapper)
    {
      if (context instanceof Activity)
      {
        return (Activity) context;
      }
      context = ((ContextWrapper) context).getBaseContext();
    }
    return null;
  }
}
