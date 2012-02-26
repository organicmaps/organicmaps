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
import android.view.View;

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

    @Override
    protected Integer doInBackground(Void... p)
    {
      // If negative, stores error code
      int bytesCopied;
      do
      {
        bytesCopied = nativeCopyNextFile();
        if (bytesCopied > 0)
          publishProgress(new Integer(bytesCopied));
        else if (bytesCopied < 0)
          return new Integer(bytesCopied);
      } while (bytesCopied != 0);

      return new Integer(ERR_COPIED_SUCCESSFULLY);
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
      mwmActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
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

  @Override
  protected Dialog onCreateDialog(int totalBytesToCopy)
  {
    m_dialog = new ProgressDialog(this);
    m_dialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
    m_dialog.setMessage(String.format(getString(R.string.loading), getString(R.string.app_name)));
    m_dialog.setCancelable(false);
    m_dialog.setIndeterminate(false);
    m_dialog.setMax(totalBytesToCopy);
    return m_dialog;
  }

  public void onCopyResourcesProgress(int copiedBytes)
  {
    if (m_dialog != null)
      m_dialog.setProgress(copiedBytes);
  }

  private native void nativeMoveMaps(String fromFolder, String toFolder);
  private native int nativeGetBytesToCopy(String m_apkPath, String m_sdcardPath);
  private native int nativeCopyNextFile();
}
