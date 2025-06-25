package app.organicmaps.util;

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
  public static @Nullable byte[] readFileSafe(String filePath)
  {
    return nativeReadFileSafe(filePath);
  }

  /**
   * Deletes the specified file on the native file thread (Platform::Thread::File)
   * Blocks until complete.
   * @return true if deletion succeeds, false otherwise
   */
  public static boolean deleteFileSafe(String filePath)
  {
    return nativeDeleteFileSafe(filePath);
  }

  /**
   * Moves the srcPath to destPath on the native file thread (Platform::Thread::File)
   * Blocks until complete.
   * @return true if move succeeds, false otherwise
   */
  public static boolean moveFileSafe(String srcPath, String destPath)
  {
    return nativeMoveFileSafe(srcPath, destPath);
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

  private static native byte[] nativeReadFileSafe(String filePath);

  private static native boolean nativeDeleteFileSafe(String filePath);

  private static native boolean nativeMoveFileSafe(String src, String dest);

  private static native byte[] nativeCalculateFileSha1(String filePath);

  private static native byte[] nativeCalculateSha1(byte[] bytes);
}
