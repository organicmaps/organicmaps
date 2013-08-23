package com.mapswithme.maps;

import java.io.Serializable;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;

public class MapStorage
{
  public static final int GROUP = -2;
  public static final int COUNTRY = -1;

  /// This constants should be equal with storage/storage.hpp, storage::TStatus
  public static final int ON_DISK = 0;
  public static final int NOT_DOWNLOADED = 1;
  public static final int DOWNLOAD_FAILED = 2;
  public static final int DOWNLOADING = 3;
  public static final int IN_QUEUE = 4;
  public static final int UNKNOWN = 5;
  public static final int ON_DISK_OUT_OF_DATE = 6;

  public interface Listener
  {
    public void onCountryStatusChanged(Index idx);
    public void onCountryProgress(Index idx, long current, long total);
  };

  public static class Index implements Serializable
  {
    private static final long serialVersionUID = 1L;

    int mGroup;
    int mCountry;
    int mRegion;

    public Index()
    {
      mGroup = -1;
      mCountry = -1;
      mRegion = -1;
    }

    public Index(int group, int country, int region)
    {
      mGroup = group;
      mCountry = country;
      mRegion = region;
    }

    public boolean isRoot()
    {
      return (mGroup == -1 && mCountry == -1 && mRegion == -1);
    }

    public boolean isValid()
    {
      return !isRoot();
    }

    public Index getChild(int position)
    {
      final Index ret = new Index(mGroup, mCountry, mRegion);

      if (ret.mGroup == -1) ret.mGroup = position;
      else if (ret.mCountry == -1) ret.mCountry = position;
      else
      {
        assert(ret.mRegion == -1);
        ret.mRegion = position;
      }

      return ret;
    }

    public Index getParent()
    {
      final Index ret = new Index(mGroup, mCountry, mRegion);

      if (ret.mRegion != -1) ret.mRegion = -1;
      else if (ret.mCountry != -1) ret.mCountry = -1;
      else
      {
        assert(ret.mGroup != -1);
        ret.mGroup = -1;
      }

      return ret;
    }

    public boolean isEqual(Index idx)
    {
      return (mGroup == idx.mGroup && mCountry == idx.mCountry && mRegion == idx.mRegion);
    }

    public boolean isChild(Index idx)
    {
      return (idx.getParent().isEqual(this));
    }

    public int getPosition()
    {
      if (mRegion != -1) return mRegion;
      else if (mCountry != -1) return mCountry;
      else return mGroup;
    }

    @Override
    public String toString()
    {
      return ("Index(" + mGroup + ", " + mCountry + ", " + mRegion + ")");
    }
  }

  public native String countryFileNameByIndex(Index idx);
  public native int countriesCount(Index idx);
  public native int countryStatus(Index idx);
  public native long countryLocalSizeInBytes(Index idx);
  public native long countryRemoteSizeInBytes(Index idx);
  public native String countryName(Index idx);
  public native String countryFlag(Index idx);

  public native void downloadCountry(Index idx);
  public native void deleteCountry(Index idx);

  public native Index findIndexByFile(String name);

  public native void showCountry(Index idx);

  public native int subscribe(Listener l);
  public native void unsubscribe(int slotId);

  private native String[] nativeGetMapsWithoutSearch();


  public MapStorage()
  {
  }

  private void runDownloadCountries(Index[] indexes)
  {
    for (int i = 0; i < indexes.length; ++i)
    {
      if (indexes[i] != null)
        downloadCountry(indexes[i]);
    }
  }

  public interface UpdateFunctor
  {
    public void doUpdate();
    public void doCancel();
  }

  public boolean updateMaps(int msgID, Context context, final UpdateFunctor fn)
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

    String msg = context.getString(msgID);
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
