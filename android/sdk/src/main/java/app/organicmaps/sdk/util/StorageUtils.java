package app.organicmaps.sdk.util;

import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.system.OsConstants;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Queue;
import java.util.concurrent.LinkedBlockingQueue;

public class StorageUtils
{
  private static final String TAG = StorageUtils.class.getSimpleName();

  public static boolean isDirWritable(File dir)
  {
    final String path = dir.getPath();
    Logger.d(TAG, "Checking for writability " + path);

    // Keep the individual checks for diagnostics, but let direct probes decide whether
    // the operations used by map storage actually work. In particular, canRead() and
    // canWrite() are only advisory on some document-provider and FUSE-backed volumes.
    boolean success = true;
    if (!dir.isDirectory())
    {
      Logger.w(TAG, "Not a directory: " + path);
      success = false;
    }
    if (!dir.exists())
    {
      Logger.w(TAG, "Not exists: " + path);
      success = false;
    }
    if (!dir.canWrite())
      Logger.w(TAG, "Not writable: " + path);
    if (!dir.canRead())
      Logger.w(TAG, "Not readable: " + path);
    if (dir.list() == null)
    {
      Logger.w(TAG, "Not listable: " + path);
      success = false;
    }

    final File newDir = new File(dir, "om_test_dir");
    final String newPath = newDir.getPath();
    if (newDir.delete())
      Logger.i(TAG, "Deleting existing test file/dir: " + newPath);
    if (newDir.exists())
      Logger.w(TAG, "Existing test file/dir is not deleted (not empty?): " + newPath);
    if (!newDir.mkdir())
    {
      Logger.w(TAG, "Failed to create the test dir: " + newPath);
      success = false;
    }
    if (!newDir.exists())
    {
      Logger.w(TAG, "The test dir doesn't exist: " + newPath);
      success = false;
    }
    if (!newDir.delete())
    {
      Logger.w(TAG, "Failed to delete the test dir: " + newPath);
      success = false;
    }

    // Probe the operations map downloads depend on: create, write, close, and read back
    // a file. EPERM from close() is ignored exactly as the downloader ignores it.
    final File testFile = new File(dir, "om_test_file");
    final String testFilePath = testFile.getPath();
    if (testFile.delete())
      Logger.i(TAG, "Deleted the existing test file: " + testFilePath);
    // The timestamp makes the payload differ between runs, so that a stack silently
    // keeping a previous probe's file instead of the new one fails the read-back.
    final byte[] payload = (testFilePath + ' ' + System.nanoTime()).getBytes(StandardCharsets.UTF_8);
    boolean written = false;
    RandomAccessFile writer = null;
    try
    {
      // The same open mode as the downloader uses; "rw" doesn't truncate, hence setLength().
      writer = new RandomAccessFile(testFile, "rw");
      writer.setLength(0);
      writer.write(payload);
      written = true;
    }
    catch (IOException e)
    {
      // A full volume is not an unwritable one: an SD card packed with maps works again
      // as soon as a map is deleted, while marking it read-only takes it out of the
      // storage picker for good.
      if (isOutOfSpace(e))
        Logger.w(TAG, "No space left for the test file: " + testFilePath, e);
      else
      {
        Logger.w(TAG, "Failed to write the test file: " + testFilePath, e);
        success = false;
      }
    }
    finally
    {
      try
      {
        Utils.closeIgnoringEperm(writer, "the written test file " + testFilePath);
      }
      catch (IOException e)
      {
        // Deferred write errors, a full volume included, are reported by close() on FUSE
        // and network file systems. The content can't be trusted, so skip the read-back.
        written = false;
        if (isOutOfSpace(e))
          Logger.w(TAG, "No space left to close the test file: " + testFilePath, e);
        else
        {
          Logger.w(TAG, "Failed to close the written test file: " + testFilePath, e);
          success = false;
        }
      }
    }
    if (written)
    {
      try
      {
        if (!fileHasExpectedContent(testFile, payload))
        {
          Logger.w(TAG, "Read back different content from the test file: " + testFilePath);
          success = false;
        }
      }
      catch (IOException e)
      {
        Logger.w(TAG, "Failed to read the test file back: " + testFilePath, e);
        success = false;
      }
    }
    if (testFile.exists() && !testFile.delete())
    {
      Logger.w(TAG, "Failed to delete the test file: " + testFilePath);
      success = false;
    }

    return success;
  }

  static boolean fileHasExpectedContent(@NonNull File file, @NonNull byte[] expected) throws IOException
  {
    final RandomAccessFile reader = new RandomAccessFile(file, "r");
    try
    {
      if (reader.length() != expected.length)
        return false;
      final byte[] actual = new byte[expected.length];
      reader.readFully(actual);
      return Arrays.equals(expected, actual);
    }
    finally
    {
      Utils.closeIgnoringEperm(reader, "the read test file " + file.getPath());
    }
  }

  static boolean isOutOfSpace(@NonNull IOException error)
  {
    return Utils.isErrno(error, OsConstants.ENOSPC) || Utils.isErrno(error, OsConstants.EDQUOT);
  }

  @NonNull
  public static String getApkPath(@NonNull Context context)
  {
    try
    {
      return Utils.getApplicationInfo(context.getPackageManager(), Config.getApplicationId(), 0).sourceDir;
    }
    catch (final PackageManager.NameNotFoundException e)
    {
      Logger.e(TAG, "Can't get apk path from PackageManager", e);
      return "";
    }
  }

  @NonNull
  public static String addTrailingSeparator(@NonNull String dir)
  {
    if (!dir.endsWith(File.separator))
      return dir + File.separator;
    return dir;
  }

  @NonNull
  public static String getSettingsPath(@NonNull Context context)
  {
    return addTrailingSeparator(context.getFilesDir().getAbsolutePath());
  }

  @NonNull
  public static String getPrivatePath(@NonNull Context context)
  {
    return addTrailingSeparator(context.getFilesDir().getAbsolutePath());
  }

  @NonNull
  public static String getTempPath(@NonNull Context context)
  {
    return addTrailingSeparator(context.getCacheDir().getAbsolutePath());
  }

  public static boolean createDirectory(@NonNull final String path)
  {
    final File directory = new File(path);
    if (!directory.exists() && !directory.mkdirs())
    {
      final String errMsg = "Can't create directory " + path;
      Logger.e(TAG, errMsg);
      return false;
    }
    return true;
  }

  public static void requireDirectory(@Nullable final String path) throws IOException
  {
    if (!createDirectory(path))
      throw new IOException("Can't create directory " + path);
  }

  static private boolean copyFile(InputStream from, OutputStream to) throws IOException
  {
    if (from == null || to == null)
      return false;

    byte[] buf = new byte[4 * 1024];
    int len;
    while ((len = from.read(buf)) > 0)
      to.write(buf, 0, len);

    return true;
  }

  /**
   * Copy data from a URI into a local file.
   * @param resolver content resolver
   * @param from a source URI.
   * @param to a destination file
   * @return true on success and false if the provider recently crashed.
   * @throws IOException - if I/O error occurs.
   */
  public static boolean copyFile(@NonNull ContentResolver resolver, @NonNull Uri from, @NonNull File to)
      throws IOException
  {
    try (InputStream in = resolver.openInputStream(from))
    {
      try (OutputStream out = new FileOutputStream(to))
      {
        return copyFile(in, out);
      }
    }
  }

  public static boolean copyFile(@NonNull ContentResolver resolver, @NonNull Uri from, @NonNull Uri to)
      throws IOException
  {
    try (InputStream in = resolver.openInputStream(from))
    {
      try (OutputStream out = resolver.openOutputStream(to))
      {
        return copyFile(in, out);
      }
    }
  }
  /**
   * Recursively lists all movable files in the directory.
   */
  public static void listFilesRecursively(File dir, String prefix, FilenameFilter filter, ArrayList<String> relPaths)
  {
    File[] list = dir.listFiles();
    if (list == null)
    {
      Logger.w(TAG, "listFilesRecursively listFiles() returned null for " + dir.getPath());
      return;
    }

    for (File file : list)
    {
      if (file.isDirectory())
      {
        listFilesRecursively(file, prefix + file.getName() + File.separator, filter, relPaths);
        continue;
      }
      String name = file.getName();
      if (filter.accept(dir, name))
        relPaths.add(prefix + name);
    }
  }

  /**
   * Returns 0 in case of the error or if no files have passed the filter.
   */
  public static long getDirSizeRecursively(File dir, FilenameFilter fileFilter)
  {
    final File[] list = dir.listFiles();
    if (list == null)
    {
      Logger.w(TAG, "getDirSizeRecursively listFiles() returned null for " + dir.getPath());
      return 0;
    }

    long dirSize = 0;
    for (File child : list)
    {
      if (child.isDirectory())
        dirSize += getDirSizeRecursively(child, fileFilter);
      else if (fileFilter.accept(dir, child.getName()))
        dirSize += child.length();
    }
    return dirSize;
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  public static void removeEmptyDirectories(File dir)
  {
    final File[] list = dir.listFiles();
    if (list == null)
      return;
    for (File file : list)
    {
      if (!file.isDirectory())
        continue;
      removeEmptyDirectories(file);
      file.delete();
    }
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  public static boolean removeFilesInDirectory(File dir, File[] files)
  {
    try
    {
      for (File file : files)
      {
        if (file != null)
          file.delete();
      }
      removeEmptyDirectories(dir);
      return true;
    }
    catch (Exception e)
    {
      e.printStackTrace();
      return false;
    }
  }

  @FunctionalInterface
  public interface UriVisitor {
    void visit(Uri uri);
  }

  /**
   * Recursive lists all files in the given URI.
   * @param contentResolver contentResolver instance
   * @param rootUri root URI to scan
   */
  public static void listContentProviderFilesRecursively(ContentResolver contentResolver, Uri rootUri,
                                                         UriVisitor filter)
  {
    Uri rootDir =
        DocumentsContract.buildChildDocumentsUriUsingTree(rootUri, DocumentsContract.getTreeDocumentId(rootUri));
    Queue<Uri> directories = new LinkedBlockingQueue<>();
    directories.add(rootDir);
    while (!directories.isEmpty())
    {
      Uri dir = directories.remove();

      try (Cursor cur = contentResolver.query(dir,
                                              new String[] {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                                                            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                                                            DocumentsContract.Document.COLUMN_MIME_TYPE},
                                              null, null, null))
      {
        while (cur.moveToNext())
        {
          final String docId = cur.getString(0);
          final String name = cur.getString(1);
          final String mime = cur.getString(2);
          Logger.d(TAG, "docId: " + docId + ", name: " + name + ", mime: " + mime);

          if (mime.equals(DocumentsContract.Document.MIME_TYPE_DIR))
          {
            final Uri uri = DocumentsContract.buildChildDocumentsUriUsingTree(rootUri, docId);
            directories.add(uri);
          }
          else
          {
            final Uri uri = DocumentsContract.buildDocumentUriUsingTree(rootUri, docId);
            filter.visit(uri);
          }
        }
      }
    }
  }
}
