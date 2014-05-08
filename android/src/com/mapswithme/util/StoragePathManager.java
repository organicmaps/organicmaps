package com.mapswithme.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.AsyncTask;
import android.os.Environment;
import android.os.StatFs;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckedTextView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;

public class StoragePathManager
{
  public static class StorageItem
  {
    public final String m_path;
    public final long   m_size;

    StorageItem(String path, long size)
    {
      m_path = path;
      m_size = size;
    }

    @Override
    public boolean equals(Object o)
    {
      if (o == this)
        return true;
      if (o == null)
        return false;
      StorageItem other = (StorageItem) o;
      return m_size == other.m_size || m_path.equals(other.m_size);
    }

    @Override
    public int hashCode()
    {
      return Long.valueOf(m_size).hashCode();
    }
  }

  public interface SetStoragePathListener
  {
    void MoveFilesFinished(String newPath);

    void MoveFilesFailed();
  }

  abstract public static class StoragePathAdapter extends BaseAdapter
  {
    abstract public void onItemClick(int position);
  };

  // / ListView adapter
  private class StoragePathAdapterImpl extends StoragePathAdapter
  {
    // / @name Different row types.
    // @{
    private static final int     TYPE_HEADER   = 0;
    private static final int     TYPE_ITEM     = 1;
    private static final int     HEADERS_COUNT = 1;
    private static final int     TYPES_COUNT   = 2;
    // @}

    private final LayoutInflater m_inflater;
    private final Activity       m_context;
    private final int            m_listItemHeight;

    private List<StorageItem>    m_items       = null;
    private int                  m_current     = -1;
    private long                 m_sizeNeeded;

    public StoragePathAdapterImpl(Activity context)
    {
      m_context = context;
      m_inflater = m_context.getLayoutInflater();

      m_listItemHeight = (int) Utils.getAttributeDimension(context, android.R.attr.listPreferredItemHeight);
    }

    @Override
    public int getItemViewType(int position)
    {
      return (position == 0 ? TYPE_HEADER : TYPE_ITEM);
    }

    @Override
    public int getViewTypeCount()
    {
      return TYPES_COUNT;
    }

    @Override
    public int getCount()
    {
      return (m_items != null ? m_items.size() + HEADERS_COUNT : HEADERS_COUNT);
    }

    @Override
    public StorageItem getItem(int position)
    {
      return (position == 0 ? null : m_items.get(getIndexFromPos(position)));
    }

    @Override
    public long getItemId(int position)
    {
      return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent)
    {
      // 1. It's a strange thing, but when I tried to use setClickable,
      // all the views become nonclickable.
      // 2. I call setMinimumHeight(listPreferredItemHeight)
      // because standard item's height is unknown.

      switch (getItemViewType(position))
      {
        case TYPE_HEADER:
        {
          if (convertView == null)
          {
            convertView = m_inflater.inflate(android.R.layout.simple_list_item_1, null);
            convertView.setMinimumHeight(m_listItemHeight);
          }

          final TextView v = (TextView) convertView;
          v.setText(m_context.getString(R.string.maps) + ": " + getSizeString(m_sizeNeeded));
          break;
        }

        case TYPE_ITEM:
        {
          final int index = getIndexFromPos(position);
          final StorageItem item = m_items.get(index);

          if (convertView == null)
          {
            convertView = m_inflater.inflate(android.R.layout.simple_list_item_single_choice, null);
            convertView.setMinimumHeight(m_listItemHeight);
          }

          final CheckedTextView v = (CheckedTextView) convertView;
          v.setText(item.m_path + ": " + getSizeString(item.m_size));
          v.setChecked(index == m_current);
          v.setEnabled((index == m_current) || isAvailable(index));
          break;
        }
      }

      return convertView;
    }

    @Override
    public void onItemClick(int position)
    {
      final int index = getIndexFromPos(position);
      if (isAvailable(index))
        onStorageItemClick(index);
    }

    public void updateList(ArrayList<StorageItem> items, int currentItemIndex, long dirSize)
    {
      m_sizeNeeded = dirSize;
      m_items = items;
      m_current = currentItemIndex;

      notifyDataSetChanged();
    }

    @SuppressLint("DefaultLocale")
    private String getSizeString(long size)
    {
      final String arrS[] = { "Kb", "Mb", "Gb" };

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

      // left 1 digit after the comma and add postfix string
      return String.format("%.1f %s", (double) size / (double) current, arrS[i]);
    }

    private boolean isAvailable(int index)
    {
      assert (index >= 0 && index < m_items.size());
      return ((m_current != index) && (m_items.get(index).m_size >= m_sizeNeeded));
    }

    private int getIndexFromPos(int position)
    {
      final int index = position - HEADERS_COUNT;
      assert (index >= 0 && index < m_items.size());
      return index;
    }
  }

  private static String          TAG                = "StoragePathManager";
  private static String          MWM_DIR_POSTFIX    = "/MapsWithMe/";

  private BroadcastReceiver      m_externalListener;
  private BroadcastReceiver      m_internalListener;
  private Activity               m_context          = null;
  private ArrayList<StorageItem> m_items            = null;
  private StoragePathAdapterImpl m_adapter          = null;
  private int                    m_currentItemIndex = -1;

  public void StartExtStorageWatching(Activity context, BroadcastReceiver listener)
  {
    m_context = context;
    m_externalListener = listener;

    m_internalListener = new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        if (m_externalListener != null)
          m_externalListener.onReceive(context, intent);

        UpdateExternalStorages();
      }
    };

    final IntentFilter filter = new IntentFilter();
    filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_REMOVED);
    filter.addAction(Intent.ACTION_MEDIA_EJECT);
    filter.addAction(Intent.ACTION_MEDIA_SHARED);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTABLE);
    filter.addAction(Intent.ACTION_MEDIA_CHECKING);
    filter.addAction(Intent.ACTION_MEDIA_NOFS);
    filter.addDataScheme("file");

    m_context.registerReceiver(m_internalListener, filter);
    UpdateExternalStorages();
  }

  public StoragePathAdapter GetAdapter()
  {
    if (m_adapter == null)
    {
      m_adapter = new StoragePathAdapterImpl(m_context);
      UpdateExternalStorages();
    }

    return m_adapter;
  }

  public void StopExtStorageWatching()
  {
    if (m_internalListener != null)
    {
      m_context.unregisterReceiver(m_internalListener);
      m_internalListener = null;
      m_externalListener = null;
      m_adapter = null;
    }
  }

  public boolean HasMoreThanOnceStorage()
  {
    return m_items.size() > 1;
  }

  public void UpdateExternalStorages()
  {
    ArrayList<String> pathes = new ArrayList<String>();

    if (Utils.apiEqualOrGreaterThan(android.os.Build.VERSION_CODES.KITKAT))
    {
      File[] files = m_context.getExternalFilesDirs(null);
      if (files != null)
      {
        File primaryStorageDir = m_context.getExternalFilesDir(null);
        for (File f : files)
        {
          // On kitkat and Greater we ignore private folder on primary storage
          // like "PrimaryStorage/Android/data/com.mapswithme.maps.pro/file/"
          // because
          // we can write to root of PrimaryStorage/
          if (f != null && !f.equals(primaryStorageDir))
            pathes.add(f.getPath());
        }
      }
    }

    ParseMountFile("/etc/vold.conf", VOLD_MODE, pathes);
    ParseMountFile("/etc/vold.fstab", VOLD_MODE, pathes);
    ParseMountFile("/system/etc/vold.fstab", VOLD_MODE, pathes);
    ParseMountFile("/proc/mounts", MOUNTS_MODE, pathes);

    ArrayList<StorageItem> items = new ArrayList<StorageItem>();
    for (String path : pathes)
      AddStorage(path, items);

    AddStorage(Environment.getExternalStorageDirectory().getAbsolutePath(), items);

    String writableDir = GetWritableDirRoot();

    StorageItem currentItem = AddStorage(writableDir, items);
    LinkedHashSet<StorageItem> itemsSet = new LinkedHashSet<StorageItem>(items);
    if (currentItem != null)
      itemsSet.remove(currentItem);

    m_items = new ArrayList<StorageItem>(itemsSet);
    if (currentItem != null)
    {
      m_items.add(0, currentItem);
      m_currentItemIndex = m_items.indexOf(currentItem);
    }
    else
      m_currentItemIndex = -1;
    if (m_adapter != null)
      m_adapter.updateList(m_items, m_currentItemIndex, GetMWMDirSize());
  }

  // / @name Assume that MapsWithMe folder doesn't have inner folders and
  // symbolic links.
  // @{
  public long GetMWMDirSize()
  {
    String writableDir = Framework.GetWritableDir();

    final File dir = new File(writableDir);
    assert (dir.exists());
    assert (dir.isDirectory());

    long size = 0;
    for (final File f : dir.listFiles())
    {
      assert (f.isFile());
      size += f.length();
    }

    return size;
  }

  // @}

  public boolean MoveBookmarks()
  {
    ArrayList<String> pathes = new ArrayList<String>();
    if (Utils.apiEqualOrGreaterThan(android.os.Build.VERSION_CODES.KITKAT))
    {
      ParseMountFile("/etc/vold.conf", VOLD_MODE, pathes);
      ParseMountFile("/etc/vold.fstab", VOLD_MODE, pathes);
      ParseMountFile("/system/etc/vold.fstab", VOLD_MODE, pathes);
      ParseMountFile("/proc/mounts", MOUNTS_MODE, pathes);
    }

    ArrayList<String> approvedPathes = new ArrayList<String>();
    for (String path : pathes)
    {
      String mwmPath = path + MWM_DIR_POSTFIX;
      File f = new File(mwmPath);
      if (f.exists() || f.canRead() || f.isDirectory())
        approvedPathes.add(mwmPath);
    }
    final String settingsDir = Framework.GetSettingsDir();
    final String writableDir = Framework.GetWritableDir();
    final String bookmarkDir = Framework.GetBookmarksDir();
    final String bookmarkFileExt = Framework.GetBookmarkFileExt();

    LinkedHashSet<File> bookmarks = new LinkedHashSet<File>();
    if (!settingsDir.equals(writableDir))
      approvedPathes.add(writableDir);

    for (String path : approvedPathes)
    {
      if (!path.equals(settingsDir))
        AccamulateFiles(path, bookmarkFileExt, bookmarks);
    }

    long bookmarksSize = 0;
    for (File f : bookmarks)
      bookmarksSize += f.length();

    if (GetFreeBytesAtPath(bookmarkDir) < bookmarksSize)
      return false;

    for (File f : bookmarks)
    {
      String name = f.getName();
      name = name.replace(bookmarkFileExt, "");
      name = nativeGenerateUniqueBookmarkName(name);
      try
      {
        copyFile(f, new File(name));
        f.delete();
      }
      catch (IOException e)
      {
        return false;
      }
    }

    Framework.ReloadBookmarks();

    return true;
  }

  private void AccamulateFiles(final String dirPath, final String filesExtension, Set<File> result)
  {
    File f = new File(dirPath);
    File[] bookmarks = f.listFiles(new FileFilter()
    {
      @Override
      public boolean accept(File pathname)
      {
        return pathname.getName().endsWith(filesExtension);
      }
    });

    result.addAll(Arrays.asList(bookmarks));
  }

  private void onStorageItemClick(int index)
  {
    final StorageItem oldItem = (m_currentItemIndex != -1) ? m_items.get(m_currentItemIndex) : null;
    final StorageItem item = m_items.get(index);
    final String path = GetItemFullPath(item);

    final File f = new File(path);
    if (!f.exists() && !f.mkdirs())
    {
      Log.e(TAG, "Can't create directory: " + path);
      return;
    }

    new AlertDialog.Builder(m_context).setCancelable(false).setTitle(R.string.move_maps)
        .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            SetStoragePathImpl(m_context, new StoragePathManager.SetStoragePathListener()
            {
              @Override
              public void MoveFilesFinished(String newPath)
              {
                UpdateExternalStorages();
              }

              @Override
              public void MoveFilesFailed()
              {
                UpdateExternalStorages();
              }
            }, item, oldItem, R.string.wait_several_minutes);

            dlg.dismiss();
          }
        }).setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            dlg.dismiss();
          }
        }).create().show();
  }

  public void CheckWritableDir(Context context, SetStoragePathListener listener)
  {
    if (Utils.apiLowerThan(android.os.Build.VERSION_CODES.KITKAT))
      return;

    final String settingsDir = Framework.GetSettingsDir();
    final String writableDir = Framework.GetWritableDir();

    if (settingsDir.equals(writableDir))
      return;

    if (IsDirWritable(writableDir))
      return;

    final long size = GetMWMDirSize();
    for (StorageItem item : m_items)
    {
      if (item.m_size > size)
      {
        SetStoragePathImpl(context, listener, item, new StorageItem(GetWritableDirRoot(), 0),
            R.string.kitkat_optimization_in_progress);
        return;
      }
    }

    listener.MoveFilesFailed();
  }

  private void SetStoragePathImpl(Context context, SetStoragePathListener listener, StorageItem newStorage,
      StorageItem oldStorage, int messageId)
  {
    MoveFilesTask task = new MoveFilesTask(context, listener, newStorage, oldStorage, messageId);
    task.execute("");
  }

  static private boolean DoMoveMaps(StorageItem newStorage, StorageItem oldStorage)
  {
    String fullOldPath = GetItemFullPath(oldStorage);
    String fullNewPath = GetItemFullPath(newStorage);
    File oldDir = new File(fullOldPath);
    File newDir = new File(fullNewPath);
    if (!newDir.exists())
      newDir.mkdir();

    assert (IsDirWritable(fullNewPath));
    assert (newDir.isDirectory());
    assert (oldDir.isDirectory());

    final String[] extensions = Framework.GetMovableFilesExt();

    File[] internalFiles = oldDir.listFiles(new FileFilter()
    {

      @Override
      public boolean accept(File pathname)
      {
        for (String postfix : extensions)
        {
          if (pathname.getName().endsWith(postfix))
            return true;
        }
        return false;
      }
    });

    try
    {
      for (File moveFile : internalFiles)
        copyFile(moveFile, new File(fullNewPath + moveFile.getName()));
    }
    catch (IOException e)
    {
      for (File moveFile : internalFiles)
        new File(fullNewPath + moveFile.getName()).delete();
      return false;
    }

    Framework.SetWritableDir(fullNewPath);

    for (File moveFile : internalFiles)
      moveFile.delete();

    return true;
  }

  private static void copyFile(File source, File dest) throws IOException
  {
    FileChannel inputChannel = null;
    FileChannel outputChannel = null;
    try
    {
      inputChannel = new FileInputStream(source).getChannel();
      outputChannel = new FileOutputStream(dest).getChannel();
      outputChannel.transferFrom(inputChannel, 0, inputChannel.size());
    }
    finally
    {
      inputChannel.close();
      outputChannel.close();
    }
  }

  private class MoveFilesTask extends AsyncTask<String, Void, Boolean>
  {
    private final ProgressDialog         m_dlg;
    private final StorageItem            m_newStorage;
    private final StorageItem            m_oldStorage;
    private final SetStoragePathListener m_listener;

    public MoveFilesTask(Context context, SetStoragePathListener listener, StorageItem newStorage,
        StorageItem oldStorage, int messageID)
    {
      m_newStorage = newStorage;
      m_oldStorage = oldStorage;
      m_listener = listener;

      m_dlg = new ProgressDialog(context);
      m_dlg.setMessage(context.getString(messageID));
      m_dlg.setProgressStyle(ProgressDialog.STYLE_SPINNER);
      m_dlg.setIndeterminate(true);
      m_dlg.setCancelable(false);
    }

    @Override
    protected void onPreExecute()
    {
      m_dlg.show();
    }

    @Override
    protected Boolean doInBackground(String... params)
    {
      return DoMoveMaps(m_newStorage, m_oldStorage);
    }

    @Override
    protected void onPostExecute(Boolean result)
    {
      // Using dummy try-catch because of the following:
      // http://stackoverflow.com/questions/2745061/java-lang-illegalargumentexception-view-not-attached-to-window-manager
      try
      {
        m_dlg.dismiss();
      }
      catch (final Exception e)
      {}

      if (result)
        m_listener.MoveFilesFinished(m_newStorage.m_path);
      else
        m_listener.MoveFilesFailed();

      UpdateExternalStorages();
    }
  }

  static private StorageItem AddStorage(String path, ArrayList<StorageItem> items)
  {
    try
    {
      final File f = new File(path + "/");
      if (f.exists() && f.isDirectory() && f.canWrite())
      {
        if (!IsDirWritable(path))
          return null;

        final long size = GetFreeBytesAtPath(path);

        if (size > 0)
        {
          final StorageItem item = new StorageItem(path, size);
          items.add(item);
          return item;
        }
      }
    }
    catch (final IllegalArgumentException ex)
    {
      // Suppress exceptions for unavailable storages.
      Log.i(TAG, "StatFs error for storage: " + path);
    }

    return null;
  }

  private static int VOLD_MODE   = 1;
  private static int MOUNTS_MODE = 2;

  // http://stackoverflow.com/questions/8151779/find-sd-card-volume-label-on-android
  // http://stackoverflow.com/questions/5694933/find-an-external-sd-card-location
  // http://stackoverflow.com/questions/14212969/file-canwrite-returns-false-on-some-devices-although-write-external-storage-pe
  static private void ParseMountFile(String file, int mode, ArrayList<String> pathes)
  {
    Log.i(TAG, "Parsing " + file);

    BufferedReader reader = null;
    try
    {
      reader = new BufferedReader(new FileReader(file));

      while (true)
      {
        final String line = reader.readLine();
        if (line == null)
          break;

        // standard regexp for all possible whitespaces (space, tab, etc)
        final String[] arr = line.split("\\s+");

        // split may return empty first strings
        int start = 0;
        while (start < arr.length && arr[start].length() == 0)
          ++start;

        if (arr.length - start > 3)
        {
          if (arr[start + 0].charAt(0) == '#')
            continue;

          if (mode == VOLD_MODE)
          {
            Log.i(TAG, "Label = " + arr[start + 1] + "; Path = " + arr[start + 2]);

            if (arr[start + 0].startsWith("dev_mount"))
              pathes.add(arr[start + 2]);
          }
          else
          {
            assert (mode == MOUNTS_MODE);
            Log.i(TAG, "Label = " + arr[start + 0] + "; Path = " + arr[start + 1]);

            final String prefixes[] = { "tmpfs", "/dev/block/vold", "/dev/fuse", "/mnt/media_rw" };
            for (final String s : prefixes)
              if (arr[start + 0].startsWith(s))
                pathes.add(arr[start + 1]);
          }
        }
      }
    }
    catch (final IOException e)
    {
      Log.w(TAG, "Can't read file: " + file);
    }
    finally
    {
      Utils.closeStream(reader);
    }
  }

  @SuppressWarnings("deprecation")
  @SuppressLint("NewApi")
  static private long GetFreeBytesAtPath(String path)
  {
    long size = 0;
    try
    {
      if (Utils.apiLowerThan(android.os.Build.VERSION_CODES.GINGERBREAD))
      {
        final StatFs stat = new StatFs(path);
        size = stat.getAvailableBlocks() * (long) stat.getBlockSize();
      }
      else
        size = new File(path).getFreeSpace();
    }
    catch (RuntimeException e)
    {
      Log.d(TAG, e.getMessage());
      Log.d(TAG, e.getStackTrace().toString());
    }

    return size;
  }

  static private boolean IsDirWritable(String path)
  {
    final File f = new File(path + "/testDir");
    f.mkdir();
    // we can't only call canWrite, because on KitKat (Samsung S4) this return
    // true
    // for sdcard but actually it's read only
    if (f.exists())
    {
      f.delete();
      return true;
    }

    return false;
  }

  static private String GetItemFullPath(StorageItem item)
  {
    return item.m_path + MWM_DIR_POSTFIX;
  }

  static private String GetWritableDirRoot()
  {
    String writableDir = Framework.GetWritableDir();
    int index = writableDir.lastIndexOf(MWM_DIR_POSTFIX);
    if (index != -1)
      writableDir = writableDir.substring(0, index);

    return writableDir;
  }

  static private native String nativeGenerateUniqueBookmarkName(String baseName);
}
