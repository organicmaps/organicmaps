package app.organicmaps.sdk.util;

public class ByteUtils
{
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

  public static boolean contains(byte[] source, byte[] target) {
    return contains(source, target, 0);
  }

  public static boolean contains(byte[] source, byte[] target, int source_offset)
  {
    // Check if 'source' array contains bytes from 'target' at some offset.
    if (target.length > source.length + source_offset)
      return false;

    for (int i = source_offset; i < source.length; i++)
      if (startsWith(source, target, i))
        return true;

    return false;
  }
}
