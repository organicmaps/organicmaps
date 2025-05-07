package app.organicmaps.sdk;

import androidx.annotation.Keep;

public class DownloadResourcesLegacyActivity
{
  // Error codes, should match the same codes in JNI
  public static final int ERR_DOWNLOAD_SUCCESS = 0;
  public static final int ERR_DISK_ERROR = -1;
  public static final int ERR_NOT_ENOUGH_FREE_SPACE = -2;
  public static final int ERR_STORAGE_DISCONNECTED = -3;
  public static final int ERR_DOWNLOAD_ERROR = -4;
  public static final int ERR_NO_MORE_FILES = -5;
  public static final int ERR_FILE_IN_PROGRESS = -6;

  public interface Listener
  {
    // Called by JNI.
    @Keep
    void onProgress(int percent);

    // Called by JNI.
    @Keep
    void onFinish(int errorCode);
  }

  public static native int nativeGetBytesToDownload();
  public static native int nativeStartNextFileDownload(Listener listener);
  public static native void nativeCancelCurrentFile();
}
