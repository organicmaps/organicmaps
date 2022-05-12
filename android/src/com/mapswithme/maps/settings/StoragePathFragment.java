package com.mapswithme.maps.settings;

import android.app.ProgressDialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.dialog.DialogUtils;
import com.mapswithme.util.Constants;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;

import java.io.File;
import java.util.List;
import java.util.Locale;

public class StoragePathFragment extends BaseSettingsFragment
    implements OnBackPressListener
{
  private TextView mHeader;
  private ListView mList;

  private StoragePathAdapter mAdapter;
  private StoragePathManager mPathManager;

  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_prefs_storage;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    View root = super.onCreateView(inflater, container, savedInstanceState);
    mPathManager = new StoragePathManager(requireActivity());
    mAdapter = new StoragePathAdapter(requireActivity());

    mHeader = root.findViewById(R.id.header);
    mList = root.findViewById(R.id.list);
    mList.setOnItemClickListener((parent, view, position, id) -> changeStorage(position));
    mList.setAdapter(mAdapter);

    return root;
  }

  @Override
  public void onResume()
  {
    super.onResume();
    mPathManager.startExternalStorageWatching((items, idx) -> updateList());
    mPathManager.updateExternalStorages();
    updateList();
  }

  @Override
  public void onPause()
  {
    mPathManager.stopExternalStorageWatching();
    super.onPause();
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
    mAdapter.update(mPathManager.getStorageItems(), mPathManager.getCurrentStorageIndex(), dirSize);
  }

  /**
   * Asks for user confirmation and starts to move to a new storage location.
   *
   * @param newIndex new storage location index
   */
  public void changeStorage(int newIndex)
  {
    final int currentIndex = mPathManager.getCurrentStorageIndex();
    if (newIndex == currentIndex || currentIndex == -1 || !mAdapter.isStorageBigEnough(newIndex))
      return;

    final List<StorageItem> items = mPathManager.getStorageItems();
    final String oldPath = items.get(currentIndex).getFullPath();
    final String newPath = items.get(newIndex).getFullPath();

    new AlertDialog.Builder(requireActivity())
        .setCancelable(false)
        .setTitle(R.string.move_maps)
        .setPositiveButton(R.string.ok, (dlg, which) -> moveStorage(newPath, oldPath))
        .setNegativeButton(R.string.cancel, (dlg, which) -> dlg.dismiss())
        .create()
        .show();
  }

  /**
   * Shows a progress dialog and runs a move files thread.
   */
  private void moveStorage(@NonNull final String newPath, @NonNull final String oldPath)
  {
    final ProgressDialog dialog = DialogUtils.createModalProgressDialog(requireActivity(), R.string.wait_several_minutes);
    dialog.show();

    ThreadPool.getStorage().execute(() ->
      {
        final boolean result = mPathManager.moveStorage(newPath, oldPath);

        UiThread.run(() ->
          {
           if (dialog.isShowing())
             dialog.dismiss();

           if (!result)
           {
             final String message = "Error moving maps files";
             new AlertDialog.Builder(requireActivity())
                 .setTitle(message)
                 .setPositiveButton(R.string.report_a_bug,
                                    (dlg, which) -> Utils.sendBugReport(requireActivity(), message))
                 .show();
           }
           mPathManager.updateExternalStorages();
           updateList();
          });
      });
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
