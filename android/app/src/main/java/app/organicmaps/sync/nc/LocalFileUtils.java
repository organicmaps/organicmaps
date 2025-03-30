package app.organicmaps.sync.nc;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.Nullable;

import app.organicmaps.bookmarks.data.BookmarkManager;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

// TODO when working on the actual project (if selected) refactor out such classes into interfaces for testing
public class LocalFileUtils
{
  public static void deleteFileSafe(String filePath)
  {
    new Handler(Looper.getMainLooper()).post(() -> {
      BookmarkManager.nativeDeleteBmCategoryPermanently(filePath);
    });
  }

  public static void reloadBookmarksList(String filePath)
  {
    new Handler(Looper.getMainLooper()).post(() -> {
      BookmarkManager.nativeReloadBookmark(filePath);
    });
  }

//  public static void duplicateFile(File file, File newFile, boolean retainOriginal) throws IOException
//  {
//    try (FileInputStream fis = new FileInputStream(file))
//    {
//      try (FileOutputStream fos = new FileOutputStream(newFile))
//      {
//        byte[] buf = new byte[4 * 1024];
//        int len;
//        while ((len = fis.read(buf)) > 0)
//          fos.write(buf, 0, len);
//      }
//    }
//    if (!retainOriginal)
//      deleteFileSafe(file.getAbsolutePath());
//    reloadBookmarksList(file.getAbsolutePath());
//  }
  public static void safeMoveFile(File file, File newFile) throws IOException
  {
    if (!file.renameTo(newFile))
      throw new IOException("Attempt to move file failed.");
    deleteFileSafe(file.getAbsolutePath()); // TODO maybe add another native function that doesn't attempt (in vain) to delete at the original file path
    reloadBookmarksList(newFile.getAbsolutePath());
  }

  /**
   *
   * @param file The file to add the suffix to
   * @param suffix if null, then it's taken to be "_1" // TODO make it so that it turns _1 into _2 instead of _1_1
   * @return The new file (after moving)
   */
  public static File safeAddSuffixToFilename(File file, @Nullable String suffix) throws Exception
  {
    if (suffix == null)
      suffix = "_1";
    else
      suffix = sanitizeFilenameComponent(suffix);
    String fileName = file.getName();
    File newFile = new File(file.getParent(),
        fileName.substring(0, fileName.lastIndexOf(".")) + suffix + fileName.substring(fileName.lastIndexOf(".")));
    safeMoveFile(file, newFile);
    return newFile;
  }

  public static String sanitizeFilenameComponent(String component)
  {
    // The list of illegal characters was taken from https://docs.nextcloud.com/server/latest/developer_manual/client_apis/android_library/examples.html#tips
    return component.replaceAll("[/<>:\"|?*]", "");
  }
}
