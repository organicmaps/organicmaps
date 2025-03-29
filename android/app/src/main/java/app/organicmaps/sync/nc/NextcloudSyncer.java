package app.organicmaps.sync.nc;

import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;

import androidx.annotation.NonNull;

import app.organicmaps.util.ConnectionState;
import app.organicmaps.util.log.Logger;

import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArraySet;

public enum NextcloudSyncer
{
  INSTANCE;

  private static final long SYNC_DELAY_MS = 10_000;
  private static final String TAG = NextcloudSyncer.class.getSimpleName();

  final PollHelper mPollHelper;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private NextcloudPreferences ncprefs;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private CopyOnWriteArraySet<String> changedFilesSet; // thread-safe, persistent
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private File bookmarksDir;

  private NextcloudSyncer()
  {
    HandlerThread handlerThread = new HandlerThread("NcSyncerThread");
    handlerThread.start();
    mPollHelper = new PollHelper(SYNC_DELAY_MS, new Handler(handlerThread.getLooper()), this::performSync);
  }

  private void saveChangedFiles()
  {
    HashSet<String> newSet = new HashSet<>(changedFilesSet);
    ncprefs.setChangedFiles(newSet);
  }

  private void performSync()
  {
    if (!ConnectionState.INSTANCE.isWifiConnected())
      return; // TODO add options for enabling sync over mobile data
    Logger.d(TAG, "performSync called");
  }

  public void initialize(@NonNull Context context)
  {
    bookmarksDir = new File(context.getFilesDir(), "bookmarks"); // TODO replace this with the alternative (utilize ::NotifyBookmarksChanged when
    //                                                                    bookmarks loading hasn't finished) approach to not have to rely on this hardcoded path
    ncprefs = new NextcloudPreferences(context);
    Set<String> changedFiles = ncprefs.getChangedFiles();
    if (changedFiles == null)
    {
      changedFilesSet = new CopyOnWriteArraySet<>(getAllBookmarkFilePaths());
      saveChangedFiles();
    }
    else
      changedFilesSet = new CopyOnWriteArraySet<>(changedFiles);
  }

  private ArrayList<String> getAllBookmarkFilePaths()
  {
    ArrayList<String> bookmarkFilepaths = new ArrayList<>();
    try{
      for(File file : Objects.requireNonNull(bookmarksDir.listFiles()))
      {
        String filepath = file.getAbsolutePath();
        if (filepath.endsWith(".kml"))
          bookmarkFilepaths.add(filepath);
      }
    } catch (Exception e)
    {
      Logger.e(TAG, "Error listing bookmark files", e);
    }
    return bookmarkFilepaths;
  }

  public void onLogout()
  {
    changedFilesSet = new CopyOnWriteArraySet<>(getAllBookmarkFilePaths());
    saveChangedFiles();
  }

  public void onLocalChange(String filePath)
  {
    if (filePath.isEmpty())
      return;
    android.util.Log.e("pocstuff", "yayyyy: " + filePath);
    if (!changedFilesSet.contains(filePath))
    {
      changedFilesSet.add(filePath);
      saveChangedFiles();
    }
  }

  // should be called only after the framework has been initialized
  public void resumeSync()
  {
    if (ncprefs.getSyncEnabled())
      mPollHelper.start();
  }

  public void pauseSync()
  {
    mPollHelper.stop();
  }
}
