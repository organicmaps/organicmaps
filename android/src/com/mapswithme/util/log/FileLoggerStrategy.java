package com.mapswithme.util.log;

import android.content.Context;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.support.annotation.NonNull;
import android.util.Log;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.Utils;
import net.jcip.annotations.Immutable;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.Executor;

@Immutable
class FileLoggerStrategy implements LoggerStrategy
{
  private static final String TAG = FileLoggerStrategy.class.getSimpleName();
  @NonNull
  private final String mFilePath;
  @NonNull
  private final Executor mExecutor;

  FileLoggerStrategy(@NonNull String filePath, @NonNull Executor executor)
  {
    mFilePath = filePath;
    mExecutor = executor;
  }

  @Override
  public void v(String tag, String msg)
  {
    write("V/" + tag + ": " + msg);
  }

  @Override
  public void v(String tag, String msg, Throwable tr)
  {
    write("V/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  @Override
  public void d(String tag, String msg)
  {
    write("D/" + tag + ": " + msg);
  }

  @Override
  public void d(String tag, String msg, Throwable tr)
  {
    write("D/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  @Override
  public void i(String tag, String msg)
  {
    write("I/" + tag + ": " + msg);
  }

  @Override
  public void i(String tag, String msg, Throwable tr)
  {

    write("I/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  @Override
  public void w(String tag, String msg)
  {
    write("W/" + tag + ": " + msg);
  }

  @Override
  public void w(String tag, String msg, Throwable tr)
  {
    write("W/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  @Override
  public void w(String tag, Throwable tr)
  {
    write("D/" + tag + ": " + Log.getStackTraceString(tr));
  }

  @Override
  public void e(String tag, String msg)
  {
    write("E/" + tag + ": " + msg);
  }

  @Override
  public void e(String tag, String msg, Throwable tr)
  {
    write("E/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  private void write(@NonNull final String data)
  {
    mExecutor.execute(new WriteTask(mFilePath, data, Thread.currentThread().getName()));
  }

  static class WriteTask implements Runnable
  {
    private static final int MAX_SIZE = 3000000;
    @NonNull
    private final String mFilePath;
    @NonNull
    private final String mData;
    @NonNull
    private final String mCallingThread;

    private WriteTask(@NonNull String filePath, @NonNull String data, @NonNull String callingThread)
    {
      mFilePath = filePath;
      mData = data;
      mCallingThread = callingThread;
    }

    @Override
    public void run()
    {
      FileWriter fw = null;
      try
      {
        File file = new File(mFilePath);
        if (!file.exists() || file.length() > MAX_SIZE)
        {
          fw = new FileWriter(file, false);
          writeSystemInformation(fw);
        }
        else
        {
          fw = new FileWriter(mFilePath, true);
        }
        DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US);
        fw.write(formatter.format(new Date()) + " " + mCallingThread + ": " + mData + "\n");
      }
      catch (IOException e)
      {
        Log.e(TAG, "Failed to write the string: " + mData, e);
        Log.i(TAG, "Is logs folder existent: " + StorageUtils.ensureLogsFolderExistence());
      }
      finally
      {
        if (fw != null)
          try
          {
            fw.close();
          }
          catch (IOException e)
          {
            Log.e(TAG, "Failed to close file: " + mData, e);
          }
      }
    }

    static void writeSystemInformation(FileWriter fw) throws IOException
    {
      fw.write("Android version: " + Build.VERSION.SDK_INT + "\n");
      fw.write("Device: " + Utils.getFullDeviceModel() + "\n");
      fw.write("App version: " + BuildConfig.APPLICATION_ID + " " + BuildConfig.VERSION_NAME + "\n");
      fw.write("Installation ID: " + Utils.getInstallationId() + "\n");
      fw.write("Locale : " + Locale.getDefault());
      fw.write("\nNetworks : ");
      final ConnectivityManager manager = (ConnectivityManager) MwmApplication.get().getSystemService(Context.CONNECTIVITY_SERVICE);
      for (NetworkInfo info : manager.getAllNetworkInfo())
        fw.write(info.toString());
      fw.write("\nLocation providers: ");
      final LocationManager locMngr = (android.location.LocationManager) MwmApplication.get().getSystemService(Context.LOCATION_SERVICE);
      for (String provider: locMngr.getProviders(true))
        fw.write(provider + " ");
      fw.write("\n\n");
    }
  }
}
