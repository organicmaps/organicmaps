package com.mapswithme.maps;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import android.app.ListActivity;
import android.os.Bundle;
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

  private List<String> m_choices = new ArrayList<String>();
  private List<String> m_pathes = new ArrayList<String>();
  private int m_checked;

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

    return (size / current + arrS[i]);
  }

  private void parseMountFile(String file)
  {
    BufferedReader reader = null;
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

          Log.i(TAG, "Label = " + arr[1] + "; Path = " + arr[2]);

          StatFs stat = new StatFs(arr[2]);
          final long size = stat.getAvailableBlocks() * stat.getBlockSize();
          Log.i(TAG, "Available size = " + size);

          if (size > 0)
          {
            m_choices.add(arr[1] + ", " + getSizeString(size) + " available");
            m_pathes.add(arr[2]);
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

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    parseMountFile("/etc/vold.conf");
    parseMountFile("/etc/vold.fstab");

    setListAdapter(new ArrayAdapter<String>(this,
        android.R.layout.simple_list_item_single_choice, m_choices));

    final ListView lv = getListView();
    lv.setItemsCanFocus(false);
    lv.setChoiceMode(ListView.CHOICE_MODE_SINGLE);

    // set current selection by current storage path
    final String current = nativeGetStoragePath();
    for (int i = 0; i < m_pathes.size(); ++i)
      if (m_pathes.get(i).equals(current))
      {
        lv.setItemChecked(i, true);
        m_checked = i;
        break;
      }
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id)
  {
    if (position != m_checked)
    {
      final String s = m_pathes.get(position) + "/MapsWithMe/";

      File f = new File(s);
      if (!f.exists() && !f.mkdirs())
      {
        Log.e(TAG, "Can't create directory: " + s);
        return;
      }

      Log.i(TAG, "Transfer data to storage: " + s);
      if (nativeSetStoragePath(s))
      {
        l.setItemChecked(position, true);

        /*
        final CheckedTextView old = getViewByPos(l, m_checked);
        if (old != null)
          old.setChecked(false);
        ((CheckedTextView) v).setChecked(true);
         */

        m_checked = position;
      }
    }
  }

  private native String nativeGetStoragePath();
  private native boolean nativeSetStoragePath(String newPath);
}
