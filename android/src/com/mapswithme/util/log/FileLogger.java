package com.mapswithme.util.log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class FileLogger extends Logger
{
  private static final SimpleDateFormat sDate = new SimpleDateFormat("yyyy.MM.dd - HH:mm:ss", Locale.US);
  private OutputStreamWriter mWriter;

  private void write(String str)
  {
    try
    {
      mWriter.write(sDate.format(new Date()) + ": " + str + "\n");
      mWriter.flush();
    } catch (IOException e)
    {
      e.printStackTrace();
    }
  }

  public FileLogger(String fileName)
  {
    try
    {
      mWriter = new OutputStreamWriter(new FileOutputStream(new File(fileName), true));
    } catch (FileNotFoundException e)
    {
      e.printStackTrace();
    }
  }

  @Override
  public synchronized void d(String message)
  {
    write(message);
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
