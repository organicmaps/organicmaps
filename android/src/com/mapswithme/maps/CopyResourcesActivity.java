package com.mapswithme.maps;

import java.io.File;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;

// MWM resources are zipped in the apk inside /assets folder.
// MWM code currently can't randomly read zip files, so they're copied
// on the first start to the sdcard and used from there
public class CopyResourcesActivity extends Activity
{
  private ProgressDialog m_dialog = null;

  // Error codes, should match the same codes in JNI
  private static final int ERR_COPIED_SUCCESSFULLY = 0;
  private static final int ERR_NOT_ENOUGH_MEMORY = -1;
  private static final int ERR_NOT_ENOUGH_FREE_SPACE = -2;
  private static final int ERR_STORAGE_DISCONNECTED = -3;

  // Copies assets files to external folder on sdcard in the background
  public class CopyResourcesTask extends AsyncTask<Void, Integer, Integer>
  {
    private final String m_apkPath;
    private final String m_sdcardPath;

    CopyResourcesTask(String apkPath, String sdcardPath)
    {
      m_apkPath = apkPath;
      m_sdcardPath = sdcardPath;
    }

    @Override
    protected void onPreExecute()
    {
      // Check if we need to perform any copying
      final int bytes = nativeGetBytesToCopy(m_apkPath, m_sdcardPath);
      if (bytes > 0)
      {
        // Display copy progress dialog
        CopyResourcesActivity.this.showDialog(bytes);
      }
      else if (bytes == 0)
      {
        // All files are in place, continue with UI initialization
        cancel(true);
      }
      else
      {
        // Display error dialog in UI, very rare case
        CopyResourcesActivity.this.onCopyTaskFinished(ERR_NOT_ENOUGH_MEMORY);
      }
    }

    @Override
    protected void onPostExecute(Integer result)
    {
      CopyResourcesActivity.this.onCopyTaskFinished(result);
    }

    @Override
    protected void onCancelled()
    {
      // In our logic cancelled means that files shouldn't be copied,
      // So we just proceed to main UI initialization
      CopyResourcesActivity.this.onCopyTaskFinished(ERR_COPIED_SUCCESSFULLY);
    }

    @Override
    protected void onProgressUpdate(Integer... copiedBytes)
    {
      CopyResourcesActivity.this.onCopyResourcesProgress(copiedBytes[0]);
    }

    // If negative, stores error code
    // Total number of copied bytes
    int m_bytesCopied = 0;

    protected void onFileProgress(int size, int pos)
    {
      publishProgress(m_bytesCopied + pos);
    }

    @Override
    protected Integer doInBackground(Void... p)
    {
      do
      {
        m_bytesCopied = nativeCopyNextFile(this);
        if (m_bytesCopied > 0)
          publishProgress(m_bytesCopied);
        else if (m_bytesCopied < 0)
          return m_bytesCopied;
      } while (m_bytesCopied != 0);

      return ERR_COPIED_SUCCESSFULLY;
    }
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    // Set the same layout as for MWMActivity
    setContentView(R.layout.map);

    final String extPath = MWMActivity.getDataStoragePath();
    // Create sdcard folder if it doesn't exist
    new File(extPath).mkdirs();
    // Used to migrate from v2.0.0 to 2.0.1
    nativeMoveMaps(MWMActivity.getExtAppDirectoryPath("files"), extPath);
    // All copy checks are made in the background task
    new CopyResourcesTask(MWMActivity.getApkPath(this), extPath).execute();
  }

  public void onCopyTaskFinished(int result)
  {
    if (result == ERR_COPIED_SUCCESSFULLY)
    {
      // Continue with Main UI initialization (MWMActivity)
      Intent mwmActivityIntent = new Intent(this, MWMActivity.class);
      // Disable animation because MWMActivity should appear exactly over this one
      mwmActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
      startActivity(mwmActivityIntent);
    }
    else
    {
      // Hide loading progress
      if (m_dialog != null)
        m_dialog.dismiss();

      int errMsgId;
      switch (result)
      {
      case ERR_NOT_ENOUGH_FREE_SPACE: errMsgId = R.string.not_enough_free_space_on_sdcard; break;
      case ERR_STORAGE_DISCONNECTED: errMsgId = R.string.disconnect_usb_cable; break;
      default: errMsgId = R.string.not_enough_memory;
      }
      // Display Error dialog with close button
      new AlertDialog.Builder(this).setMessage(errMsgId)
        .setPositiveButton(R.string.close, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int whichButton) {
              CopyResourcesActivity.this.finish();
              CopyResourcesActivity.this.moveTaskToBack(true);
          }})
        .create().show();
    }
  }

  private String cutProgressString(final String str, int current, int total)
  {
    int len = current * str.length() / total;
    if (len <= 0)
      len = 0;
    else if (len > str.length())
      len = str.length();
    return str.substring(0, len);
  }

  private String m_progressString;

  @Override
  protected Dialog onCreateDialog(int totalBytesToCopy)
  {
    m_progressString = getString(R.string.loading);

    m_dialog = new ProgressDialog(this);
    m_dialog.setMessage(cutProgressString(m_progressString, 0, totalBytesToCopy));
    m_dialog.setCancelable(false);
    m_dialog.setIndeterminate(true);
    m_dialog.setMax(totalBytesToCopy);
    return m_dialog;
  }

  public void onCopyResourcesProgress(int copiedBytes)
  {
    if (m_dialog != null)
    {
      m_dialog.setMessage(cutProgressString(m_progressString, copiedBytes, m_dialog.getMax()));
      m_dialog.setProgress(copiedBytes);
    }
  }

  private native void nativeMoveMaps(String fromFolder, String toFolder);
  private native int nativeGetBytesToCopy(String m_apkPath, String m_sdcardPath);
  private native int nativeCopyNextFile(Object observer);
}
