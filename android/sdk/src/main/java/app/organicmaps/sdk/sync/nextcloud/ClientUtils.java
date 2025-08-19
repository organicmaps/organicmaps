package app.organicmaps.sdk.sync.nextcloud;

import android.net.Uri;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.util.log.Logger;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.Locale;
import java.util.Objects;

public class ClientUtils
{
  private static final String TAG = ClientUtils.class.getSimpleName();
  private static final DateTimeFormatter TIME_FORMATTER = DateTimeFormatter.RFC_1123_DATE_TIME;

  static Uri getBmUri(Uri bookmarksDir, String fileName)
  {
    return bookmarksDir.buildUpon().appendPath(fileName).build();
  }

  static long toTimestampMs(String value) throws SyncOpException.UnexpectedException
  {
    try
    {
      return ZonedDateTime.parse(value, TIME_FORMATTER).toInstant().toEpochMilli();
    }
    catch (DateTimeParseException e)
    {
      // This should never really happen for Nextcloud.
      return parseDateFallback(value);
    }
  }

  private static long parseDateFallback(String value) throws SyncOpException.UnexpectedException
  {
    // List of formats taken from
    //   https://github.com/nextcloud/android-library/blob/master/library/src/main/java/com/owncloud/android/lib/common/network/WebdavUtils.java
    Logger.w(TAG, "Fallback date parsing requested for \"" + value + "\".");
    for (String format :
         new String[] {"yyyy-MM-dd'T'HH:mm:ss'Z'", "EEE, dd MMM yyyy HH:mm:ss zzz", "yyyy-MM-dd'T'HH:mm:ss.sss'Z'",
                       "yyyy-MM-dd'T'HH:mm:ssZ", "EEE MMM dd HH:mm:ss zzz yyyy", "EEEEEE, dd-MMM-yy HH:mm:ss zzz",
                       "EEE MMMM d HH:mm:ss yyyy", "yyyy-MM-dd hh:mm:ss"})
    {
      try
      {
        SimpleDateFormat sdf = new SimpleDateFormat(format, Locale.US);
        return Objects.requireNonNull(sdf.parse(value)).toInstant().toEpochMilli();
      }
      catch (ParseException ignored)
      {}
    }
    throw new SyncOpException.UnexpectedException("Unable to parse date " + value);
  }

  static void throwIfUnsuccessful(int statusCode) throws SyncOpException
  {
    if (statusCode / 100 != 2)
      throw statusCode == 401 ? new SyncOpException.AuthExpiredException() : unexpectedHttp(statusCode);
  }

  static SyncOpException.UnexpectedException unexpectedHttp(int statusCode)
  {
    return new SyncOpException.UnexpectedException("HTTP " + statusCode);
  }
}
