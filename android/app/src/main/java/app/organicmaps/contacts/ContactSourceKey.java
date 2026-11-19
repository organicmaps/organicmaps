package app.organicmaps.contacts;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

final class ContactSourceKey
{
  private ContactSourceKey() {}

  static String hash(String key)
  {
    try
    {
      byte[] bytes = MessageDigest.getInstance("SHA-256").digest(key.getBytes(StandardCharsets.UTF_8));
      char[] hex = "0123456789abcdef".toCharArray();
      StringBuilder result = new StringBuilder(bytes.length * 2);
      for (byte value : bytes)
        result.append(hex[(value & 255) >>> 4]).append(hex[value & 15]);
      return result.toString();
    }
    catch (NoSuchAlgorithmException e)
    {
      // SHA-256 is mandatory on Android; never persist a raw provider identifier as a fallback.
      throw new IllegalStateException(e);
    }
  }
}
