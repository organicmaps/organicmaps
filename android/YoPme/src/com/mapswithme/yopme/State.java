package com.mapswithme.yopme;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

import android.content.Context;

import com.mapswithme.yopme.BackscreenActivity.Mode;
import com.mapswithme.yopme.map.MapData;

public class State implements Serializable
{
  private static final long serialVersionUID = 1L;
  private static final String FILE_NAME = "state.st";

  // state
  Mode mMode;
  MapData mData;

  public static boolean read(Context context, State state)
  {
    try
    {
      final FileInputStream fis = context.openFileInput(FILE_NAME);
      final ObjectInputStream ois = new ObjectInputStream(fis);

      state.mMode = (Mode) ois.readObject();
      state.mData = (MapData) ois.readObject();

      if (state.mData == null)
        state.mData = new MapData();

      ois.close();
      return true;
    }
    catch (final IOException ioExc)
    {
      ioExc.printStackTrace();
    }
    catch (final ClassNotFoundException cnfExc)
    {
      cnfExc.printStackTrace();
    }

    return false;
  }

  public static void write(Context context, State state)
  {
    try
    {
      final FileOutputStream fos = context.openFileOutput(FILE_NAME, Context.MODE_PRIVATE);
      final ObjectOutputStream oos = new ObjectOutputStream(fos);

      oos.writeObject(state.mMode);
      oos.writeObject(state.mData);

      oos.close();
    }
    catch (final IOException ioExc)
    {
      ioExc.printStackTrace();
    }
  }
}
