package com.mapswithme.maps.settings;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.content.Intent;
import android.content.UriPermission;
import android.net.Uri;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.documentfile.provider.DocumentFile;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesFragment;
import com.mapswithme.maps.dialog.DialogUtils;
import com.mapswithme.util.Constants;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.List;
import java.util.Locale;

public class StoragePathFragment extends BaseSettingsFragment
                              implements StoragePathManager.MoveFilesListener,
                                         OnBackPressListener,
                                         View.OnClickListener,
                                         View.OnLongClickListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.STORAGE);
  private static final String TAG = "SAF test";

  private TextView mHeader;
  private ListView mList;

  private StoragePathAdapter mAdapter;
  private final StoragePathManager mPathManager = new StoragePathManager();

  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_prefs_storage;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    View root = super.onCreateView(inflater, container, savedInstanceState);

    mHeader = root.findViewById(R.id.header);
    mHeader.setOnClickListener(this);
    mHeader.setOnLongClickListener(this);
    mList = root.findViewById(R.id.list);
    mList.setOnItemClickListener(new AdapterView.OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        mAdapter.onItemClick(position);
      }
    });

    return root;
  }

  @Override
  public void onResume()
  {
    super.onResume();
    mPathManager.startExternalStorageWatching(getActivity(), new StoragePathManager.OnStorageListChangedListener()
    {
      @Override
      public void onStorageListChanged(List<StorageItem> storageItems, int currentStorageIndex)
      {
        updateList();
      }
    }, this);

    if (mAdapter == null)
      mAdapter = new StoragePathAdapter(mPathManager, getActivity());

    updateList();
    mList.setAdapter(mAdapter);
  }

  @Override
  public void onPause()
  {
    super.onPause();
    mPathManager.stopExternalStorageWatching();
  }

  static long getWritableDirSize()
  {
    final File writableDir = new File(Framework.nativeGetWritableDir());
    if (BuildConfig.DEBUG)
    {
      if (!writableDir.exists())
        throw new IllegalStateException("Writable directory doesn't exits, can't get size.");
      if (!writableDir.isDirectory())
        throw new IllegalStateException("Writable directory isn't a directory, can't get size.");
    }

    return StorageUtils.getDirSizeRecursively(writableDir, StoragePathManager.MOVABLE_FILES_FILTER);
  }

  private void updateList()
  {
    long dirSize = getWritableDirSize();
    mHeader.setText(getString(R.string.maps) + ": " + getSizeString(dirSize));

    if (mAdapter != null)
      mAdapter.update(mPathManager.getStorageItems(), mPathManager.getCurrentStorageIndex(), dirSize);
  }

  @Override
  public void onClick(View v)
  {
    // Choose a directory using the system's file picker.
    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);

    // Optionally, specify a URI for the directory that should be opened in
    // the system file picker when it loads.
    intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, "content://com.android.externalstorage.documents/tree/primary:OrganicMaps");

    // Enable "Show SD card option"
    // http://stackoverflow.com/a/31334967/1615876
    //intent.putExtra("android.content.extra.SHOW_ADVANCED", true);

    intent.putExtra(DocumentsContract.EXTRA_EXCLUDE_SELF, true);
    startActivityForResult(intent, 333);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode,
                               Intent resultData) {
    final Context context = getActivity();
    final ContentResolver resolver = context.getContentResolver();

    if (requestCode == 333
        && resultCode == Activity.RESULT_OK) {
      // The result data contains a URI for the document or directory that
      // the user selected.
      if (resultData != null) {
        final Uri treeUri = resultData.getData();
        LOGGER.i(TAG, "Persisting user selected uri: " + treeUri);

        final int takeFlags = resultData.getFlags()
                              & (Intent.FLAG_GRANT_READ_URI_PERMISSION
                                 | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        resolver.takePersistableUriPermission(treeUri, takeFlags);
      }
    }
  }

  @Override
  public boolean onLongClick(View v)
  {
    final Context context = getActivity();
    final ContentResolver resolver = context.getContentResolver();

    List<UriPermission> permissions = resolver.getPersistedUriPermissions();
    if (permissions != null && permissions.size() > 0)
    {
      DocumentFile pickedDir = null;
      for (UriPermission uriPerm : permissions)
      {
        LOGGER.i(TAG, "Persisted uri: " + uriPerm.toString());
        DocumentFile testDir = DocumentFile.fromTreeUri(context, uriPerm.getUri());
        if (testDir != null && testDir.exists() && testDir.isDirectory()) {
          pickedDir = testDir;
        } else {
          LOGGER.i(TAG, "Something wrong with: " + uriPerm.toString());
        }
      }

      String ftime = "file-" + System.currentTimeMillis() + ".txt";
      LOGGER.i(TAG, "Creating new file: " + ftime);
      DocumentFile newFile = pickedDir.createFile("text/plain", ftime);
      try
      {
        ParcelFileDescriptor pfd = resolver.openFileDescriptor(newFile.getUri(), "w");
        FileOutputStream fileOutputStream = new FileOutputStream(pfd.getFileDescriptor());
        fileOutputStream.write(ftime.getBytes());
        fileOutputStream.close();
        pfd.close();
      }
      catch (IOException e)
      {
        e.printStackTrace();
      }

      LOGGER.i(TAG, "Scanning uri: " + pickedDir.getUri());
      for (DocumentFile file : pickedDir.listFiles())
      {
        LOGGER.i(TAG, "Found file: " + file.getName());
        try
        {
          ParcelFileDescriptor pfd = resolver.openFileDescriptor(file.getUri(), "r");
          FileInputStream fis = new FileInputStream(pfd.getFileDescriptor());
          InputStreamReader inputStreamReader = new InputStreamReader(fis);
          BufferedReader reader = new BufferedReader(inputStreamReader);
          String line = reader.readLine();
          LOGGER.i(TAG, "  contents: " + line);
          fis.close();
          pfd.close();

          String contents = "overwritten-" + System.currentTimeMillis();
          LOGGER.i(TAG, "Overwriting with a current timestamp: " + contents);
          pfd = resolver.openFileDescriptor(file.getUri(), "w");
          FileOutputStream fos = new FileOutputStream(pfd.getFileDescriptor());
          //fos.getChannel().truncate(0); // doesn't work on Android 5.0
          fos.write(contents.getBytes());
          fos.close();
          pfd.close();
        }
        catch (FileNotFoundException e)
        {
          e.printStackTrace();
        }
        catch (IOException e)
        {
          e.printStackTrace();
        }
      }
    }
    else
    {
      LOGGER.i(TAG, "No persisted uris found! Select a dir");
    }

    return true;
  }

  @Override
  public void moveFilesFinished(String newPath)
  {
    updateList();
  }

  @Override
  public void moveFilesFailed(int errorCode)
  {
    if (!isAdded())
      return;

    final String message = "Failed to move maps with internal error: " + errorCode;
    final Activity activity = getActivity();
    if (activity.isFinishing())
      return;

    new AlertDialog.Builder(activity)
        .setTitle(message)
        .setPositiveButton(R.string.report_a_bug,
                           (dialog, which) -> Utils.sendBugReport(activity, message)).show();
  }

  static String getSizeString(long size)
  {
    final String[] units = { "Kb", "Mb", "Gb" };

    long current = Constants.KB;
    int i = 0;
    for (; i < units.length; ++i)
    {
      final long bound = Constants.KB * current;
      if (size < bound)
        break;

      current = bound;
    }

    // left 1 digit after the comma and add postfix string
    return String.format(Locale.US, "%.1f %s", (double) size / current, units[i]);
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }
}
