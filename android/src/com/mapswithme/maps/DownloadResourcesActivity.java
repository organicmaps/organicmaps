package com.mapswithme.maps;

import java.io.File;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;

public class DownloadResourcesActivity extends Activity
{
  private static final String TAG = "DownloadResourcesActivity";
  
  private ProgressDialog mDialog = null;

  // Error codes, should match the same codes in JNI
  
  private static final int ERR_DOWNLOAD_SUCCESS = 0;
  private static final int ERR_NOT_ENOUGH_MEMORY = -1;
  private static final int ERR_NOT_ENOUGH_FREE_SPACE = -2;
  private static final int ERR_STORAGE_DISCONNECTED = -3;
  private static final int ERR_DOWNLOAD_ERROR = -4;
  private static final int ERR_NO_MORE_FILES = -5;
  private static final int ERR_FILE_IN_PROGRESS = -6;

  MWMApplication mApplication;
  
  protected void prepareFilesDownload()
  {
    // Check if we need to perform any downloading
    final int bytes = nativeGetBytesToDownload(
        mApplication.getApkPath(), 
        mApplication.getDataStoragePath());

    /// disabling screen
    disableAutomaticStandby();
    
    if (bytes > 0)
    {
      // Display copy progress dialog
      Log.w(TAG, "prepareFilesDownload, bytesToDownload:" + bytes);
      
      showDialog(bytes);
      
      if (nativeDownloadNextFile(this) == ERR_NO_MORE_FILES)
        finishFilesDownload(ERR_NO_MORE_FILES);
    }
    else if (bytes == 0)
    {
      // All files are in place, continue with UI initialization
      finishFilesDownload(ERR_DOWNLOAD_SUCCESS);
    }
    else
    {
      // Display error dialog in UI, very rare case
      finishFilesDownload(ERR_NOT_ENOUGH_MEMORY);
    }
  }

  private WakeLock mWakeLock = null;
  
  private void disableAutomaticStandby()
  {
    if (mWakeLock == null)
    {
      PowerManager pm = (PowerManager) mApplication.getSystemService(
        android.content.Context.POWER_SERVICE);
      mWakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, TAG);
      mWakeLock.acquire();
    }
  }

  private void enableAutomaticStandby()
  {
    if (mWakeLock != null)
    {
      mWakeLock.release();
      mWakeLock = null;
    }
  }
  
  
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    // Set the same layout as for MWMActivity
    setContentView(R.layout.map);

    mApplication = (MWMApplication)getApplication();

    // Create sdcard folder if it doesn't exist
    new File(mApplication.getDataStoragePath()).mkdirs();
    // Used to migrate from v2.0.0 to 2.0.1
    nativeMoveMaps(mApplication.getExtAppDirectoryPath("files"), 
                   mApplication.getDataStoragePath());

    prepareFilesDownload();
  }
    
  public void finishFilesDownload(int result)
  {
    enableAutomaticStandby();
    if (result == ERR_NO_MORE_FILES)
    {
      Log.w(TAG, "finished files download");
      
      // Continue with Main UI initialization (MWMActivity)
      Intent mwmActivityIntent = new Intent(this, MWMActivity.class);
      // Disable animation because MWMActivity should appear exactly over this one
      mwmActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
      startActivity(mwmActivityIntent);
    }
    else
    {
      // Hide loading progress
      if (mDialog != null)
        mDialog.dismiss();

      int errMsgId;
      switch (result)
      {
      case ERR_NOT_ENOUGH_FREE_SPACE: errMsgId = R.string.not_enough_free_space_on_sdcard; break;
      case ERR_STORAGE_DISCONNECTED: errMsgId = R.string.disconnect_usb_cable; break;
      case ERR_DOWNLOAD_ERROR: errMsgId = R.string.download_has_failed; break;
      default: errMsgId = R.string.not_enough_memory;
      }
      // Display Error dialog with close button
      new AlertDialog.Builder(this).setMessage(errMsgId)
        .setPositiveButton(R.string.close, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int whichButton) {
              finish();
              moveTaskToBack(true);
          }})
        .create().show();
    }
  }

  @Override
  protected Dialog onCreateDialog(int totalBytesToDownload)
  {
    mDialog = new ProgressDialog(this);

    mDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
    mDialog.setMessage(getString(R.string.loading));
    mDialog.setCancelable(true);
    mDialog.setIndeterminate(false);
    
    Log.w(TAG, "progressMax:" + totalBytesToDownload);
    
    mDialog.setMax(totalBytesToDownload);
    
    return mDialog;
  }

  public void onDownloadProgress(int currentTotal, int currentProgress, int globalTotal, int globalProgress)
  {
    Log.w(TAG, "curTotal:" + currentTotal + ", curProgress:" + currentProgress 
             + ", glbTotal:" + globalTotal + ", glbProgress:" + globalProgress);
    
    if (mDialog != null)
      mDialog.setProgress(globalProgress);
  }
  
  public void onDownloadFinished(int errorCode)  
  {
    if (errorCode == ERR_DOWNLOAD_SUCCESS)
    {
      int res = nativeDownloadNextFile(this);
      if (res == ERR_NO_MORE_FILES)
        finishFilesDownload(res);
    }
    else
      finishFilesDownload(errorCode);
  }
  
  private native void nativeMoveMaps(String fromFolder, String toFolder);
  private native int nativeGetBytesToDownload(String m_apkPath, String m_sdcardPath);
  private native int nativeDownloadNextFile(Object observer);
}
