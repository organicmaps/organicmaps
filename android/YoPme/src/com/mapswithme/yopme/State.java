package com.mapswithme.yopme;

import java.io.Closeable;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

import android.content.Context;
import android.graphics.Bitmap;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.yopme.BackscreenActivity.Mode;
import com.mapswithme.yopme.util.Utils;

public class State implements Serializable
{
  private static final long serialVersionUID = 1L;
  private static final String FILE_NAME = "state.st";

  // state
  Mode mMode;
  MWMPoint mPoint;
  Bitmap mBackscreenBitmap;

  public State(Mode mode, MWMPoint point, Bitmap backscreenBitmap)
  {
    mMode = mode;
    mPoint = point;
    mBackscreenBitmap = backscreenBitmap;
  }

  public State()
  {
    mMode = Mode.NONE;
    mPoint = null;
    mBackscreenBitmap = null;
  }

  public boolean hasPoint()
  {
    return mPoint != null;
  }

  public boolean hasBitmap()
  {
    return mBackscreenBitmap != null;
  }

  public Bitmap getBitmap()
  {
    return mBackscreenBitmap;
  }

  public Mode getMode()
  {
    return mMode;
  }

  public MWMPoint getPoint()
  {
    return mPoint;
  }

  public void setPoint(MWMPoint point)
  {
    mPoint = point;
  }

  public void setBitmap(Bitmap bitmap)
  {
    mBackscreenBitmap = bitmap;
  }

  public void setMode(Mode mode)
  {
    mMode = mode;
  }

  public synchronized static State read(Context context)
  {
    State state = null;
    Closeable closeable = null;
    try
    {
      final FileInputStream fis = context.openFileInput(FILE_NAME);
      final ObjectInputStream ois = new ObjectInputStream(fis);
      closeable = ois;

      state = (State) ois.readObject();
    }
    catch (final Exception e)
    {
      e.printStackTrace();
    }
    finally
    {
      Utils.close(closeable);
    }

    return state;
  }

  public synchronized static void write(Context context, State state)
  {
    Closeable closeable = null;
    try
    {
      final FileOutputStream fos = context.openFileOutput(FILE_NAME, Context.MODE_PRIVATE);
      final ObjectOutputStream oos = new ObjectOutputStream(fos);
      closeable = oos;

      oos.writeObject(state);

      oos.close();
    }
    catch (final Exception e)
    {
      e.printStackTrace();
    }
    finally
    {
      Utils.close(closeable);
    }
  }
}
