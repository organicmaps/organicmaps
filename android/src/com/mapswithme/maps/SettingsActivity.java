package com.mapswithme.maps;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Environment;
import android.os.StatFs;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.CheckedTextView;
import android.widget.ListView;

import com.mapswithme.util.Utils;

public class SettingsActivity extends ListActivity
{
  private static String TAG = "SettingsActivity";
  private static String MWM_DIR_POSTFIX = "/MapsWithMe/";

  private List<String> m_choices = new ArrayList<String>();
  private List<String> m_pathes = new ArrayList<String>();
  private int m_checked = -1;

  private String getSizeString(long size)
  {
    String arrS[] = { "Kb", "Mb", "Gb" };

    long current = 1024;
    int i = 0;
    for (; i < arrS.length; ++i)
    {
      final long bound = 1024 * current;
      if (size < bound)
        break;
      else
        current = bound;
    }

    return (size / current + " " + arrS[i]);
  }

  private boolean addStorage(String path, long sizeNeeded)
  {
    try
    {
      final File f = new File(path);
      if (f.exists() && f.isDirectory() && f.canWrite())
      {
        StatFs stat = new StatFs(path);
        final long size = (long)stat.getAvailableBlocks() * (long)stat.getBlockSize();
        Log.i(TAG, "Available size = " + size);

        if (size >= sizeNeeded)
        {
          m_choices.add(path + ", " + getSizeString(size));
          m_pathes.add(path);
          return true;
        }
      }
      else
        Log.i(TAG, "File error for storage: " + path);
    }
    catch (IllegalArgumentException ex)
    {
      // Suppress exceptions for unavailable storages.
      Log.i(TAG, "StatFs error for storage: " + path);
    }

    return false;
  }

  /// @return Flag that current path (1) or default path (2) is added successfully.
  private int callAddStorage(String path, String currPath, String defPath,
                             long sizeNeeded, int flag)
  {
    assert(sizeNeeded > 0);
    if (path.equals(currPath))
      sizeNeeded = 0;

    if (addStorage(path, sizeNeeded))
    {
      if (path.equals(currPath))
        flag |= 1;
      if (path.equals(defPath))
        flag |= 2;
    }

    return flag;
  }

  private void parseMountFile(String file, int mode,
                              String currPath, String defPath, long sizeNeeded)
  {
    BufferedReader reader = null;
    int addFlag = 0;

    try
    {
      reader = new BufferedReader(new FileReader(file));

      while (true)
      {
        String line = reader.readLine();
        if (line == null) break;

        String[] arr = line.split(" ");
        if (arr.length >= 3)
        {
          if (arr[0].charAt(0) == '#')
            continue;

          if (mode == 0)
          {
            // parse "vold"
            Log.i(TAG, "Label = " + arr[1] + "; Path = " + arr[2]);

            if (arr[0].startsWith("dev_mount"))
              addFlag = callAddStorage(arr[2], currPath, defPath, sizeNeeded, addFlag);
          }
          else
          {
            // parse "mounts"
            Log.i(TAG, "Label = " + arr[0] + "; Path = " + arr[1]);

            String prefixes[] = { "tmpfs", "/dev/block/vold", "/dev/fuse" };
            for (String s : prefixes)
              if (arr[0].startsWith(s))
                addFlag = callAddStorage(arr[1], currPath, defPath, sizeNeeded, addFlag);
          }
        }
      }
    }
    catch (IOException e)
    {
      Log.w(TAG, "Can't read file: " + file);
    }
    finally
    {
      Utils.closeStream(reader);
    }

    if ((addFlag & 1) == 0)
      addFlag = callAddStorage(currPath, currPath, defPath, sizeNeeded, addFlag);
    if ((addFlag & 2) == 0)
      addFlag = callAddStorage(defPath, currPath, defPath, sizeNeeded, addFlag);

    assert(addFlag == 3);
  }

  private CheckedTextView getViewByPos(ListView lv, int position)
  {
    final int child = position - lv.getFirstVisiblePosition();
    if (child < 0 || child >= lv.getChildCount())
    {
      Log.e(TAG, "Unable to get view for desired position: " + position);
      return null;
    }
    return (CheckedTextView) lv.getChildAt(child);
  }

  private String getFullPath(int position)
  {
    return m_pathes.get(position) + MWM_DIR_POSTFIX;
  }

  /// @name Assume that MapsWithMe folder doesn't have inner folders and symbolic links.
  //@{
  private long getDirSize(String name)
  {
    File dir = new File(name);
    assert(dir.exists());
    assert(dir.isDirectory());

    long size = 0;
    for (File f : dir.listFiles())
    {
      assert(f.isFile());
      size += f.length();
    }

    return (size + 1024*1024);
  }

  // delete all files (except settings.ini) in directory
  private void deleteFiles(File dir)
  {
    assert(dir.exists());
    assert(dir.isDirectory());

    for (File file : dir.listFiles())
    {
      assert(file.isFile());

      // skip settings.ini - this file should be always in one place
      if (file.getName().equalsIgnoreCase("settings.ini"))
        continue;

      if (!file.delete())
        Log.w(TAG, "Can't delete file: " + file.getName());
    }
  }
  //@}

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    final String currPath = nativeGetStoragePath();
    final String defPath = Environment.getExternalStorageDirectory().getAbsolutePath();
    Log.i(TAG, "Current and Default maps pathes: " + currPath + "; " + defPath);

    final long sizeNeeded = getDirSize(currPath + MWM_DIR_POSTFIX);
    Log.i(TAG, "Needed size for maps: " + sizeNeeded);

    //parseMountFile("/etc/vold.conf", 0, currPath, defPath, sizeNeeded);
    //parseMountFile("/etc/vold.fstab", 0, currPath, defPath, sizeNeeded);
    parseMountFile("/proc/mounts", 1, currPath, defPath, sizeNeeded);

    setListAdapter(new ArrayAdapter<String>(this,
        android.R.layout.simple_list_item_single_choice, m_choices));

    final ListView lv = getListView();
    lv.setItemsCanFocus(false);
    lv.setChoiceMode(ListView.CHOICE_MODE_SINGLE);

    // set current selection by current storage path
    for (int i = 0; i < m_pathes.size(); ++i)
      if (m_pathes.get(i).equals(currPath))
      {
        lv.setItemChecked(i, true);
        m_checked = i;
        break;
      }
  }

  @Override
  protected void onListItemClick(final ListView l, View v, final int position, long id)
  {
    if (position != m_checked)
    {
      final String path = getFullPath(position);

      File f = new File(path);
      if (!f.exists() && !f.mkdirs())
      {
        Log.e(TAG, "Can't create directory: " + path);
        return;
      }

      new AlertDialog.Builder(this)
      .setCancelable(false)
      .setTitle(R.string.move_maps)
      .setMessage(R.string.wait_several_minutes)
      .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dlg, int which)
        {
          Log.i(TAG, "Transfer data to storage: " + path);
          if (nativeSetStoragePath(path))
          {
            if (m_checked != -1)
              deleteFiles(new File(getFullPath(m_checked)));

            l.setItemChecked(position, true);

            m_checked = position;
          }
          else if (m_checked != -1)
          {
            // set check state back to m_checked item
            l.setItemChecked(m_checked, true);
          }

          dlg.dismiss();
        }
      })
      .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dlg, int which)
        {
          // set check state back to m_checked item
          if (m_checked != -1)
            l.setItemChecked(m_checked, true);

          dlg.dismiss();
        }
      })
      .create()
      .show();
    }
  }

  private native String nativeGetStoragePath();
  private native boolean nativeSetStoragePath(String newPath);
}
