package com.mapswithme.util;

import java.io.Closeable;
import java.io.IOException;

import android.util.Log;

public class Utils
{
  private static final String TAG = "Utils";

  public static void closeStream(Closeable stream)
  {
    if (stream != null)
    {
      try
      {
        stream.close();
      }
      catch (IOException e)
      {
        Log.e(TAG, "Can't close stream", e);
      }
    }
  }
}
