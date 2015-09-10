package com.mapswithme.maps;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Environment;
import android.support.annotation.StringRes;
import android.support.v7.app.AlertDialog;

import com.mapswithme.maps.settings.StoragePathManager;

import java.io.Serializable;
import java.util.HashSet;
import java.util.Set;

public enum MapStorage
{
  INSTANCE;

  public static final int GROUP = -2;
  public static final int COUNTRY = -1;

  // This constants should be equal with storage/storage_defines.hpp, storage::TStatus
  public static final int ON_DISK = 0;
  public static final int NOT_DOWNLOADED = 1;
  public static final int DOWNLOAD_FAILED = 2;
  public static final int DOWNLOADING = 3;
  public static final int IN_QUEUE = 4;
  public static final int UNKNOWN = 5;
  public static final int ON_DISK_OUT_OF_DATE = 6;
  public static final int DOWNLOAD_FAILED_OUF_OF_MEMORY = 7;

  /**
   * Callbacks are called from native code.
   */
  public interface Listener
  {
    void onCountryStatusChanged(Index idx);

    void onCountryProgress(Index idx, long current, long total);
  }

  public interface UpdateFunctor
  {
    void doUpdate();

    void doCancel();
  }

  public static class Index implements Serializable
  {
    private static final long serialVersionUID = 1L;

    private int mGroup;
    private int mCountry;
    private int mRegion;

    public Index()
    {
      mGroup = -1;
      mRegion = -1;
      mCountry = -1;
    }

    public Index(int group, int country, int region)
    {
      mGroup = group;
      mCountry = country;
      mRegion = region;
    }

    @Override
    public String toString()
    {
      return ("Index(" + mGroup + ", " + getCountry() + ", " + getRegion() + ")");
    }

    public int getCountry()
    {
      return mCountry;
    }

    public void setCountry(int mCountry)
    {
      this.mCountry = mCountry;
    }

    public int getRegion()
    {
      return mRegion;
    }

    public void setRegion(int mRegion)
    {
      this.mRegion = mRegion;
    }

    public int getGroup()
    {
      return mGroup;
    }

    public void setGroup(int group)
    {
      mGroup = group;
    }

  }

  public native int countryStatus(Index idx);

  public native long countryRemoteSizeInBytes(Index idx, int options);

  public native String countryName(Index idx);

  public native Index findIndexByFile(String name);

  public native int subscribe(Listener l);

  public native void unsubscribe(int slotId);

  private native String[] nativeGetMapsWithoutSearch();

  public static native boolean nativeMoveFile(String oldFile, String newFile);

  private void runDownloadCountries(Index[] indexes)
  {
    for (Index index : indexes)
    {
      if (index != null)
        Framework.downloadCountry(index);
    }
  }

  /**
   * Checks whether all maps contain search indexes or updates them, if not.
   *
   * @return True, if any maps where updated. False otherwise.
   */
  public boolean updateMapsWithoutSearchIndex(@StringRes int msgId, Context context, final UpdateFunctor fn)
  {
    // get map names without search index
    final String[] maps = nativeGetMapsWithoutSearch();

    if (maps.length == 0)
      return false;

    // get indexes and filter out maps that already downloading
    int count = 0;
    final Index[] indexes = new Index[maps.length];
    for (int i = 0; i < maps.length; ++i)
    {
      indexes[i] = null;

      final Index idx = findIndexByFile(maps[i]);
      if (idx != null)
      {
        final int st = countryStatus(idx);
        if (st != DOWNLOADING && st != IN_QUEUE)
        {
          indexes[i] = idx;
          ++count;
        }
      }
    }

    // all maps are already downloading
    if (count == 0)
      return false;

    String msg = context.getString(msgId);
    for (int i = 0; i < maps.length; ++i)
    {
      if (indexes[i] != null)
        msg = msg + "\n" + maps[i];
    }

    new AlertDialog.Builder(context)
        .setMessage(msg)
        .setPositiveButton(context.getString(R.string.download), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            dlg.dismiss();

            runDownloadCountries(indexes);

            fn.doUpdate();
          }
        })
        .setNegativeButton(context.getString(R.string.later), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            dlg.dismiss();

            fn.doCancel();
          }
        })
        .create()
        .show();

    return true;
  }
}
