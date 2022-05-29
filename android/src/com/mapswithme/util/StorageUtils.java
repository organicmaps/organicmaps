package com.mapswithme.util;

import android.app.Application;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.FileProvider;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.Queue;
import java.util.concurrent.LinkedBlockingQueue;


public class StorageUtils
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.STORAGE);
  private final static String TAG = StorageUtils.class.getSimpleName();
  private final static String LOGS_FOLDER = "logs";

  /**
   * Checks if external storage is available for read and write
   *
   * @return true if external storage is mounted and ready for reading/writing
   */
  private static boolean isExternalStorageWritable()
  {
    String state = Environment.getExternalStorageState();
    return Environment.MEDIA_MOUNTED.equals(state);
  }

  /**
   * Safely returns the external files directory path with the preliminary
   * checking the availability of the mentioned directory
   *
   * @return the absolute path of external files directory or null if directory can not be obtained
   * @see Context#getExternalFilesDir(String)
   */
  @Nullable
  private static String getExternalFilesDir(@NonNull Application application)
  {
    if (!isExternalStorageWritable())
      return null;

    File dir = application.getExternalFilesDir(null);
    if (dir != null)
      return dir.getAbsolutePath();

    Log.e(StorageUtils.class.getSimpleName(),
          "Cannot get the external files directory for some reasons", new Throwable());
    return null;
  }

  /**
   * Check existence of the folder for writing the logs. If that folder is absent this method will
   * try to create it and all missed parent folders.
   * @return true - if folder exists, otherwise - false
   */
  public static boolean ensureLogsFolderExistence(@NonNull Application application)
  {
    String externalDir = StorageUtils.getExternalFilesDir(application);
    if (TextUtils.isEmpty(externalDir))
      return false;

    File folder = new File(externalDir + File.separator + LOGS_FOLDER);
    boolean success = true;
    if (!folder.exists())
      success = folder.mkdirs();
    return success;
  }

  @Nullable
  public static String getLogsFolder(@NonNull Application application)
  {
    if (!ensureLogsFolderExistence(application))
      return null;

    String externalDir = StorageUtils.getExternalFilesDir(application);
    return externalDir + File.separator + LOGS_FOLDER;
  }

  @Nullable
  static String getLogsZipPath(@NonNull Application application)
  {
    String zipFile = getExternalFilesDir(application) + File.separator + LOGS_FOLDER + ".zip";
    File file = new File(zipFile);
    return file.isFile() && file.exists() ? zipFile : null;
  }

  @NonNull
  public static String getApkPath(@NonNull Application application)
  {
    try
    {
      return application.getPackageManager()
                        .getApplicationInfo(BuildConfig.APPLICATION_ID, 0).sourceDir;
    }
    catch (final PackageManager.NameNotFoundException e)
    {
      LOGGER.e(TAG, "Can't get apk path from PackageManager", e);
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
  public static String getSettingsPath(@NonNull Application application)
  {
    return addTrailingSeparator(application.getFilesDir().getAbsolutePath());
  }

  @NonNull
  public static String getPrivatePath(@NonNull Application application)
  {
    return addTrailingSeparator(application.getFilesDir().getAbsolutePath());
  }

  @NonNull
  public static String getTempPath(@NonNull Application application)
  {
    return addTrailingSeparator(application.getCacheDir().getAbsolutePath());
  }

  public static void createDirectory(@NonNull String path) throws IOException
  {
    File directory = new File(path);
    if (!directory.exists() && !directory.mkdirs())
    {
      IOException error = new IOException("Can't create directories for: " + path);
      LOGGER.e(TAG, "Can't create directories for: " + path);
      CrashlyticsUtils.INSTANCE.logException(error);
      throw error;
    }
  }

  static long getFileSize(@NonNull String path)
  {
    File file = new File(path);
    return file.length();
  }

  @NonNull
  public static Uri getUriForFilePath(@NonNull Context context, @NonNull String path)
  {
    return FileProvider.getUriForFile(context.getApplicationContext(),
                                      BuildConfig.FILE_PROVIDER_AUTHORITY, new File(path));
  }

  /**
   * Copy data from a URI into a local file.
   * @param resolve content resolver
   * @param from a source URI.
   * @param to a destination file
   * @return true on success and false if the provider recently crashed.
   * @throws IOException - if I/O error occurs.
   */
  public static boolean copyFile(@NonNull ContentResolver resolver, @NonNull Uri from, @NonNull File to) throws IOException
  {
    try (InputStream in = resolver.openInputStream(from))
    {
      if (in == null)
        return false;

      try (OutputStream out = new FileOutputStream(to))
      {
        byte[] buf = new byte[4 * 1024];
        int len;
        while ((len = in.read(buf)) > 0)
          out.write(buf, 0, len);

        return true;
      }
    }
  }

  /**
   * Copy the contents of the source file to the target file.
   * @param from a source file
   * @param to a destination file
   * @throws IOException - if I/O error occurs.
   */
  static void copyFile(File from, File to) throws IOException
  {
    int maxChunkSize = 10 * Constants.MB; // move file by smaller chunks to avoid OOM.
    FileChannel inputChannel = null, outputChannel = null;
    try
    {
      inputChannel = new FileInputStream(from).getChannel();
      outputChannel = new FileOutputStream(to).getChannel();
      long totalSize = inputChannel.size();

      for (long currentPosition = 0; currentPosition < totalSize; currentPosition += maxChunkSize)
      {
        outputChannel.position(currentPosition);
        outputChannel.transferFrom(inputChannel, currentPosition, maxChunkSize);
      }
    } finally
    {
      Utils.closeSafely(inputChannel);
      Utils.closeSafely(outputChannel);
    }
  }

  /**
   * Recursively lists all movable files in the directory.
   */
  public static void listFilesRecursively(File dir, String prefix, FilenameFilter filter, ArrayList<String> relPaths)
  {
    File[] list = dir.listFiles();
    if (list == null)
      return;

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

  public static long getDirSizeRecursively(File file, FilenameFilter fileFilter)
  {
    try
    {
      if (file.isDirectory())
      {
        long dirSize = 0;
        for (File child : file.listFiles())
          dirSize += getDirSizeRecursively(child, fileFilter);

        return dirSize;
      }

      if (fileFilter.accept(file.getParentFile(), file.getName()))
        return file.length();
    }
    catch (Exception e)
    {
      LOGGER.e(TAG, "Can't calculate file or directory size", e);
    }

    return 0;
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  public static void removeEmptyDirectories(File dir)
  {
    for (File file : dir.listFiles())
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
    } catch (Exception e)
    {
      e.printStackTrace();
      return false;
    }
  }

  @FunctionalInterface
  public interface UriVisitor
  {
    void visit(Uri uri);
  }

  /**
   * Recursive lists all files in the given URI.
   * @param contentResolver contentResolver instance
   * @param rootUri root URI to scan
   */
  public static void listContentProviderFilesRecursively(ContentResolver contentResolver, Uri rootUri, UriVisitor filter)
  {
    ArrayList<Uri> result = new ArrayList<>();

    Uri rootDir = DocumentsContract.buildChildDocumentsUriUsingTree(rootUri, DocumentsContract.getTreeDocumentId(rootUri));
    Queue<Uri> directories = new LinkedBlockingQueue<>();
    directories.add(rootDir);
    while (!directories.isEmpty())
    {
      Uri dir = directories.remove();

      try (Cursor cur = contentResolver.query(dir, new String[]{
          DocumentsContract.Document.COLUMN_DOCUMENT_ID,
          DocumentsContract.Document.COLUMN_DISPLAY_NAME,
          DocumentsContract.Document.COLUMN_MIME_TYPE
      }, null, null, null))
      {
        while (cur.moveToNext())
        {
          final String docId = cur.getString(0);
          final String name = cur.getString(1);
          final String mime = cur.getString(2);
          LOGGER.d(TAG, "docId: " + docId + ", name: " + name + ", mime: " + mime);

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
