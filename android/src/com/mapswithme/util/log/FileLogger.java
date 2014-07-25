package com.mapswithme.util.log;

import android.util.Log;

import java.io.FileWriter;
import java.io.IOException;

public class FileLogger extends Logger
{
  static private volatile FileWriter m_file = null;
  static private String m_separator;

  private void write(String str)
  {
    try
    {
      m_file.write(tag + ": " + str + m_separator);
      m_file.flush();
    } catch (IOException ex)
    {
      Log.e(tag, ex.toString());
    }
  }

  public FileLogger(String tag, String path)
  {
    this.tag = tag;
    if (m_file == null)
    {
      synchronized (FileWriter.class)
      {
        if (m_file == null)
        {
          try
          {
            m_file = new FileWriter(path + "android-logging.txt");
            m_separator = System.getProperty("line.separator");
          } catch (IOException ex)
          {
            Log.e(tag, ex.toString());
          }
        }
      }
    }
  }

  @Override
  public void d(String message)
  {
    write("Debug: " + message);
  }

  @Override
  public void e(String message)
  {
    write("Error: " + message);
  }

  @Override
  public void d(String message, Object... args)
  {
    write("Debug: " + message + join(args));
  }

  @Override
  public void e(String message, Object... args)
  {
    write("Error: " + message + join(args));
  }
}
