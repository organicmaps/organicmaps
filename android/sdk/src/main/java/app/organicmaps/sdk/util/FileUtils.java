package app.organicmaps.sdk.util;

import androidx.annotation.Nullable;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class FileUtils
{
  /**
   * Reads file's contents into memory using the native file thread (Platform::Thread::File).
   * Blocks until complete.
   * @return null if read error
   */
  @Nullable
  public static byte[] readFile(String filePath)
  {
    return nativeReadFile(filePath);
  }

  /**
   * Deletes the specified file on the native file thread (Platform::Thread::File)
   * Blocks until complete.
   * @return true if deletion succeeds, false otherwise
   */
  public static boolean deleteFile(String filePath)
  {
    return nativeDeleteFile(filePath);
  }

  /**
   * Moves the srcPath to destPath on the native file thread (Platform::Thread::File)
   * Blocks until complete.
   * @return true if move succeeds, false otherwise
   */
  public static boolean moveFile(String srcPath, String destPath)
  {
    return nativeMoveFile(srcPath, destPath);
  }

  /**
   * Computes sha1 of file contents using the native file thread (Platform::Thread::File).
   * Blocks until complete.
   * @return all zeros if read error
   */
  public static byte[] calculateFileSha1(String filePath)
  {
    return nativeCalculateFileSha1(filePath);
  }

  public static byte[] calculateSha1(byte[] bytes)
  {
    try
    {
      MessageDigest sha1 = MessageDigest.getInstance("SHA1");
      return sha1.digest(bytes);
    }
    catch (NoSuchAlgorithmException e)
    {
      return nativeCalculateSha1(bytes);
    }
  }

  private static native byte[] nativeReadFile(String filePath);

  private static native boolean nativeDeleteFile(String filePath);

  private static native boolean nativeMoveFile(String src, String dest);

  private static native byte[] nativeCalculateFileSha1(String filePath);

  private static native byte[] nativeCalculateSha1(byte[] bytes);
}
