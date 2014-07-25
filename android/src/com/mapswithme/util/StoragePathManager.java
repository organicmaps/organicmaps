package com.mapswithme.util;

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
import android.os.Build;
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

public class StoragePathManager
{
  public static class StorageItem
  {
    public final String mPath;
    public final long mSize;

    StorageItem(String path, long size)
    {
      mPath = path;
      mSize = size;
    }

    @Override
    public boolean equals(Object o)
    {
      if (o == this)
        return true;
      if (o == null)
        return false;
      StorageItem other = (StorageItem) o;
      return mSize == other.mSize || mPath.equals(other.mPath);
    }

    @Override
    public int hashCode()
    {
      return Long.valueOf(mSize).hashCode();
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
  }

  // / ListView adapter
  private class StoragePathAdapterImpl extends StoragePathAdapter
  {
    // / @name Different row types.
    // @{
    private static final int TYPE_HEADER = 0;
    private static final int TYPE_ITEM = 1;
    private static final int HEADERS_COUNT = 1;
    private static final int TYPES_COUNT = 2;
    // @}

    private final LayoutInflater mInflater;
    private final Activity mContext;
    private final int mListItemHeight;

    private List<StorageItem> mItems = null;
    private int mCurrent = -1;
    private long mSizeNeeded;

    public StoragePathAdapterImpl(Activity context)
    {
      mContext = context;
      mInflater = mContext.getLayoutInflater();

      mListItemHeight = (int) Utils.getAttributeDimension(context, android.R.attr.listPreferredItemHeight);
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
      return (mItems != null ? mItems.size() + HEADERS_COUNT : HEADERS_COUNT);
    }

    @Override
    public StorageItem getItem(int position)
    {
      return (position == 0 ? null : mItems.get(getIndexFromPos(position)));
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
        if (convertView == null)
        {
          convertView = mInflater.inflate(android.R.layout.simple_list_item_1, null);
          convertView.setMinimumHeight(mListItemHeight);
        }

        final TextView v = (TextView) convertView;
        v.setText(mContext.getString(R.string.maps) + ": " + getSizeString(mSizeNeeded));
        break;

      case TYPE_ITEM:
        final int index = getIndexFromPos(position);
        final StorageItem item = mItems.get(index);

        if (convertView == null)
        {
          convertView = mInflater.inflate(android.R.layout.simple_list_item_single_choice, null);
          convertView.setMinimumHeight(mListItemHeight);
        }

        final CheckedTextView ctv = (CheckedTextView) convertView;
        ctv.setText(item.mPath + ": " + getSizeString(item.mSize));
        ctv.setChecked(index == mCurrent);
        ctv.setEnabled((index == mCurrent) || isAvailable(index));
        break;
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
      mSizeNeeded = dirSize;
      mItems = items;
      mCurrent = currentItemIndex;

      notifyDataSetChanged();
    }

    @SuppressLint("DefaultLocale")
    private String getSizeString(long size)
    {
      final String arrS[] = {"Kb", "Mb", "Gb"};

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
      assert (index >= 0 && index < mItems.size());
      return ((mCurrent != index) && (mItems.get(index).mSize >= mSizeNeeded));
    }

    private int getIndexFromPos(int position)
    {
      final int index = position - HEADERS_COUNT;
      assert (index >= 0 && index < mItems.size());
      return index;
    }
  }

  private static String TAG = "StoragePathManager";
  private static String MWM_DIR_POSTFIX = "/MapsWithMe/";

  private BroadcastReceiver mExternalListener;
  private BroadcastReceiver mInternalListener;
  private Activity mContext = null;
  private ArrayList<StorageItem> mItems = null;
  private StoragePathAdapterImpl mAdapter = null;
  private int mCurrentItemIndex = -1;

  public void StartExtStorageWatching(Activity context, BroadcastReceiver listener)
  {
    mContext = context;
    mExternalListener = listener;

    mInternalListener = new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        if (mExternalListener != null)
          mExternalListener.onReceive(context, intent);

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

    mContext.registerReceiver(mInternalListener, filter);
    UpdateExternalStorages();
  }

  public StoragePathAdapter GetAdapter()
  {
    if (mAdapter == null)
    {
      mAdapter = new StoragePathAdapterImpl(mContext);
      UpdateExternalStorages();
    }

    return mAdapter;
  }

  public void StopExtStorageWatching()
  {
    if (mInternalListener != null)
    {
      mContext.unregisterReceiver(mInternalListener);
      mInternalListener = null;
      mExternalListener = null;
      mAdapter = null;
    }
  }

  public boolean HasMoreThanOnceStorage()
  {
    return mItems.size() > 1;
  }

  public void UpdateExternalStorages()
  {
    ArrayList<String> pathes = new ArrayList<String>();

    if (Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT)
    {
      File[] files = mContext.getExternalFilesDirs(null);
      if (files != null)
      {
        File primaryStorageDir = mContext.getExternalFilesDir(null);
        for (File f : files)
        {
          // On kitkat and Greater we ignore private folder on primary storage
          // like "PrimaryStorage/Android/data/com.mapswithme.maps.pro/file/"
          // because we can write to the root of PrimaryStorage/
          if (f != null && !f.equals(primaryStorageDir))
            pathes.add(f.getPath());
        }
      }
    }

    parseMountFiles(pathes);

    ArrayList<StorageItem> items = new ArrayList<StorageItem>();
    for (String path : pathes)
      AddStorage(path, items);

    AddStorage(Environment.getExternalStorageDirectory().getAbsolutePath(), items);

    String writableDir = GetWritableDirRoot();

    StorageItem currentItem = AddStorage(writableDir, items);
    LinkedHashSet<StorageItem> itemsSet = new LinkedHashSet<StorageItem>(items);
    if (currentItem != null)
      itemsSet.remove(currentItem);

    mItems = new ArrayList<StorageItem>(itemsSet);
    if (currentItem != null)
    {
      mItems.add(0, currentItem);
      mCurrentItemIndex = mItems.indexOf(currentItem);
    }
    else
      mCurrentItemIndex = -1;
    if (mAdapter != null)
      mAdapter.updateList(mItems, mCurrentItemIndex, GetMWMDirSize());
  }

  private void parseMountFiles(ArrayList<String> pathes)
  {
    ParseMountFile("/etc/vold.conf", VOLD_MODE, pathes);
    ParseMountFile("/etc/vold.fstab", VOLD_MODE, pathes);
    ParseMountFile("/system/etc/vold.fstab", VOLD_MODE, pathes);
    ParseMountFile("/proc/mounts", MOUNTS_MODE, pathes);
  }

  /// @name Assume that MapsWithMe folder doesn't have inner folders and symbolic links.
  public long GetMWMDirSize()
  {
    final File dir = new File(Framework.nativeGetWritableDir());
    assert (dir.exists());
    assert (dir.isDirectory());

    final File[] files = dir.listFiles();
    if (files == null)
      return 0;

    long size = 0;
    for (final File f : files)
    {
      assert (f.isFile());
      size += f.length();
    }

    return size;
  }

  public boolean MoveBookmarks()
  {
    ArrayList<String> pathes = new ArrayList<String>();
    if (Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT)
      parseMountFiles(pathes);

    ArrayList<String> approvedPathes = new ArrayList<String>();
    for (String path : pathes)
    {
      String mwmPath = path + MWM_DIR_POSTFIX;
      File f = new File(mwmPath);
      if (f.exists() || f.canRead() || f.isDirectory())
        approvedPathes.add(mwmPath);
    }
    final String settingsDir = Framework.nativeGetSettingsDir();
    final String writableDir = Framework.nativeGetWritableDir();
    final String bookmarkDir = Framework.nativeGetBookmarkDir();
    final String bookmarkFileExt = Framework.nativeGetBookmarksExt();

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
      } catch (IOException e)
      {
        return false;
      }
    }

    Framework.nativeLoadbookmarks();

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
    final StorageItem oldItem = (mCurrentItemIndex != -1) ? mItems.get(mCurrentItemIndex) : null;
    final StorageItem item = mItems.get(index);
    final String path = GetItemFullPath(item);

    final File f = new File(path);
    if (!f.exists() && !f.mkdirs())
    {
      Log.e(TAG, "Can't create directory: " + path);
      return;
    }

    new AlertDialog.Builder(mContext).setCancelable(false).setTitle(R.string.move_maps)
        .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            SetStoragePathImpl(mContext, new StoragePathManager.SetStoragePathListener()
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
    if (Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.KITKAT)
      return;

    final String settingsDir = Framework.nativeGetSettingsDir();
    final String writableDir = Framework.nativeGetWritableDir();

    if (settingsDir.equals(writableDir))
      return;

    if (IsDirWritable(writableDir))
      return;

    final long size = GetMWMDirSize();
    for (StorageItem item : mItems)
    {
      if (item.mSize > size)
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

    final String[] extensions = Framework.nativeGetMovableFilesExt();

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
    } catch (IOException e)
    {
      for (File moveFile : internalFiles)
        new File(fullNewPath + moveFile.getName()).delete();
      return false;
    }

    Framework.nativeSetWritableDir(fullNewPath);

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
    } finally
    {
      inputChannel.close();
      outputChannel.close();
    }
  }

  private class MoveFilesTask extends AsyncTask<String, Void, Boolean>
  {
    private final ProgressDialog mDlg;
    private final StorageItem mNewStorage;
    private final StorageItem mOldStorage;
    private final SetStoragePathListener mListener;

    public MoveFilesTask(Context context, SetStoragePathListener listener, StorageItem newStorage,
                         StorageItem oldStorage, int messageID)
    {
      mNewStorage = newStorage;
      mOldStorage = oldStorage;
      mListener = listener;

      mDlg = new ProgressDialog(context);
      mDlg.setMessage(context.getString(messageID));
      mDlg.setProgressStyle(ProgressDialog.STYLE_SPINNER);
      mDlg.setIndeterminate(true);
      mDlg.setCancelable(false);
    }

    @Override
    protected void onPreExecute()
    {
      mDlg.show();
    }

    @Override
    protected Boolean doInBackground(String... params)
    {
      return DoMoveMaps(mNewStorage, mOldStorage);
    }

    @Override
    protected void onPostExecute(Boolean result)
    {
      // Using dummy try-catch because of the following:
      // http://stackoverflow.com/questions/2745061/java-lang-illegalargumentexception-view-not-attached-to-window-manager
      try
      {
        mDlg.dismiss();
      } catch (final Exception e)
      {}

      if (result)
        mListener.MoveFilesFinished(mNewStorage.mPath);
      else
        mListener.MoveFilesFailed();

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
    } catch (final IllegalArgumentException ex)
    {
      // Suppress exceptions for unavailable storages.
      Log.i(TAG, "StatFs error for storage: " + path);
    }

    return null;
  }

  private static int VOLD_MODE = 1;
  private static int MOUNTS_MODE = 2;

  // http://stackoverflow.com/questions/8151779/find-sd-card-volume-label-on-android
  // http://stackoverflow.com/questions/5694933/find-an-external-sd-card-location
  // http://stackoverflow.com/questions/14212969/file-canwrite-returns-false-on-some-devices-although-write-external-storage-pe
  private static void ParseMountFile(String file, int mode, ArrayList<String> pathes)
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

            final String prefixes[] = {"tmpfs", "/dev/block/vold", "/dev/fuse", "/mnt/media_rw"};
            for (final String s : prefixes)
              if (arr[start + 0].startsWith(s))
                pathes.add(arr[start + 1]);
          }
        }
      }
    } catch (final IOException e)
    {
      Log.w(TAG, "Can't read file: " + file);
    } finally
    {
      Utils.closeStream(reader);
    }
  }

  @SuppressWarnings("deprecation")
  static private long GetFreeBytesAtPath(String path)
  {
    long size = 0;
    try
    {
      if (Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.GINGERBREAD)
      {
        final StatFs stat = new StatFs(path);
        size = stat.getAvailableBlocks() * (long) stat.getBlockSize();
      }
      else
        size = new File(path).getFreeSpace();
    } catch (RuntimeException e)
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
    return item.mPath + MWM_DIR_POSTFIX;
  }

  static private String GetWritableDirRoot()
  {
    String writableDir = Framework.nativeGetWritableDir();
    int index = writableDir.lastIndexOf(MWM_DIR_POSTFIX);
    if (index != -1)
      writableDir = writableDir.substring(0, index);

    return writableDir;
  }

  static private native String nativeGenerateUniqueBookmarkName(String baseName);
}
