package com.mapswithme.util.log;

import android.app.Application;
import android.content.Context;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.support.annotation.NonNull;
import android.util.Log;

import com.mapswithme.maps.BuildConfig;
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
  @NonNull
  private final Application mApplication;

  FileLoggerStrategy(@NonNull Application application, @NonNull String filePath,
                     @NonNull Executor executor)
  {
    mApplication = application;
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
    mExecutor.execute(new WriteTask(mApplication, mFilePath, data, Thread.currentThread().getName()));
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
    @NonNull
    private final Application mApplication;

    private WriteTask(@NonNull Application application, @NonNull String filePath,
                      @NonNull String data, @NonNull String callingThread)
    {
      mApplication = application;
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
          writeSystemInformation(mApplication, fw);
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
        Log.i(TAG, "Logs folder exists: " + StorageUtils.ensureLogsFolderExistence(mApplication));
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

    static void writeSystemInformation(@NonNull Application application, @NonNull FileWriter fw)
        throws IOException
    {
      fw.write("Android version: " + Build.VERSION.SDK_INT + "\n");
      fw.write("Device: " + Utils.getFullDeviceModel() + "\n");
      fw.write("App version: " + BuildConfig.APPLICATION_ID + " " + BuildConfig.VERSION_NAME + "\n");
      fw.write("Installation ID: " + Utils.getInstallationId() + "\n");
      fw.write("Locale : " + Locale.getDefault());
      fw.write("\nNetworks : ");
      final ConnectivityManager manager = (ConnectivityManager) application.getSystemService(Context.CONNECTIVITY_SERVICE);
      for (NetworkInfo info : manager.getAllNetworkInfo())
        fw.write(info.toString());
      fw.write("\nLocation providers: ");
      final LocationManager locMngr = (android.location.LocationManager) application.getSystemService(Context.LOCATION_SERVICE);
      for (String provider: locMngr.getProviders(true))
        fw.write(provider + " ");
      fw.write("\n\n");
    }
  }
}
