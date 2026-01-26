package app.organicmaps.sdk.util;

import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;

public class ByteUtils
{
  static byte[] utf8Bom = {(byte) 0xEF, (byte) 0xBB, (byte) 0xBF};
  static byte[] utf16BeBom = {(byte) 0xFE, (byte) 0xFF};
  static byte[] utf16LeBom = {(byte) 0xFF, (byte) 0xFE};

  public static boolean startsWith(byte[] source, byte[] target)
  {
    // Check if first bytes of 'source' array match 'target' array
    return startsWith(source, target, 0);
  }

  public static boolean startsWith(byte[] source, byte[] target, int sourceOffset)
  {
    // Check if bytes of 'source' array starting from 'sourceOffset' match 'target' array
    if (target.length > source.length + sourceOffset)
      return false;

    for (int i = 0; i < target.length; i++)
      if (source[i + sourceOffset] != target[i])
        return false;
    return true;
  }

  public static boolean contains(byte[] source, byte[] target)
  {
    // Check if 'source' array contains bytes from 'target' at some offset.
    if (target.length > source.length)
      return false;

    for (int i = 0; i < source.length; i++)
      if (startsWith(source, target, i))
        return true;

    return false;
  }

  public static Charset guessEncodingByBom(byte[] buffer)
  {
    // Look for UTF_8, UTF_16BE and UTF_16LE markers on the start of the buffer.
    if (startsWith(buffer, utf8Bom))
      return StandardCharsets.UTF_8;
    if (startsWith(buffer, utf16BeBom))
      return StandardCharsets.UTF_16BE;
    if (startsWith(buffer, utf16LeBom))
      return StandardCharsets.UTF_16LE;

    return null;
  }
}
