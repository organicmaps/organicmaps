package app.organicmaps.sync;

import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import android.util.Xml;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.FileUtils;
import app.organicmaps.util.Utils;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URLDecoder;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import okhttp3.MediaType;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.ResponseBody;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

public class NextcloudSyncClient extends SyncClient
{
  private static final String TAG = NextcloudSyncClient.class.getSimpleName();
  private static final String ORGANIC_MAPS_DIRECTORY = "Organic Maps";
  private static final String BOOKMARKS_SUBFOLDER = "bookmarks";
  private static final String XMLNS_OC = "http://owncloud.org/ns";
  private static final String XMLNS_DAV = "DAV:";

  private static final DateTimeFormatter mTimeFormatter = DateTimeFormatter.RFC_1123_DATE_TIME;
  private final String mAuthorization;
  private final Uri mRootDir;
  private final Uri mBmDir;

  NextcloudSyncClient(NextcloudAuth authState)
  {
    super(authState);
    String username = authState.getLoginName();
    mAuthorization =
        "Basic " + Base64.encodeToString((username + ":" + authState.getAppPassword()).getBytes(), Base64.NO_WRAP);
    mRootDir = Uri.parse(authState.getServer())
                   .buildUpon()
                   .path("remote.php")
                   .appendPath("dav")
                   .appendPath("files")
                   .appendPath(username)
                   .appendPath(ORGANIC_MAPS_DIRECTORY)
                   .build();
    mBmDir = mRootDir.buildUpon().appendPath(BOOKMARKS_SUBFOLDER).build();
  }

  private Uri getLockfileUri()
  {
    return mRootDir.buildUpon().appendPath(".lock").build();
  }

  private Uri getBmUri(String fileName)
  {
    return mBmDir.buildUpon().appendPath(fileName).build();
  }

  @Override
  public String computeLocalFileChecksum(String filePath)
  {
    return Utils.bytesToHex(FileUtils.calculateFileSha1(filePath), true);
  }

  @Override
  public String computeLocalFileChecksum(byte[] fileBytes)
  {
    return Utils.bytesToHex(FileUtils.calculateSha1(fileBytes), true);
  }

  @Nullable
  @Override
  public String fetchBookmarksDirState() throws SyncOpException
  {
    RequestBody body = RequestBody.create("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                              + "<d:propfind xmlns:d=\"DAV:\">"
                                              + "<d:prop><d:getetag/></d:prop>"
                                              + "</d:propfind>",
                                          MediaType.get("application/xml; charset=utf-8"));

    Request request = new Request.Builder()
                          .url(mBmDir.toString())
                          .method("PROPFIND", body)
                          .header("Authorization", mAuthorization)
                          .header("Depth", "0")
                          .build();

    // Expected response format: (status code 207)
    // <?xml version="1.0"?>
    // <d:multistatus xmlns:d="DAV:" xmlns:s="http://sabredav.org/ns" xmlns:oc="http://owncloud.org/ns">
    //   <d:response>
    //     <d:href>/remote.php/dav/files/username/Organic Maps/bookmarks/</d:href>
    //     <d:propstat>
    //       <d:prop>
    //         <d:getetag>"685d7785365d6"</d:getetag>
    //       </d:prop>
    //       <d:status>HTTP/1.1 200 OK</d:status>
    //     </d:propstat>
    //   </d:response>
    // </d:multistatus>

    try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
    {
      int code = response.code();
      if (code == 404)
      {
        makeDirs();
        return null;
      }
      throwIfUnsuccessful(code);

      XmlPullParser parser = Xml.newPullParser();
      parser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, true);
      parser.setInput(Objects.requireNonNull(response.body()).byteStream(), null);

      int eventType = parser.getEventType();
      while (eventType != XmlPullParser.END_DOCUMENT)
      {
        if (eventType == XmlPullParser.START_TAG && "getetag".equals(parser.getName())
            && XMLNS_DAV.equals(parser.getNamespace()))
        {
          String eTag = trimQuotes(readText(parser));
          if (eTag != null)
            return eTag;
        }
        eventType = parser.next();
      }
      Logger.e(TAG, "Bookmark directory ETag not found");
      throw new SyncOpException.UnexpectedException();
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error trying to fetch bookmarks dir ETag.", e);
      throw new SyncOpException.NetworkException();
    }
    catch (XmlPullParserException e)
    {
      Logger.e(TAG, "Error parsing XML while trying to fetch bookmarks dir ETag.", e);
      throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
    }
  }

  private void makeDirs() throws SyncOpException
  {
    makeDir(mRootDir);
    makeDir(mBmDir);
  }

  private void makeDir(Uri dir) throws SyncOpException
  {
    // Response Status : 201 if created, 405 if already exists.
    Request request =
        new Request.Builder().url(dir.toString()).header("Authorization", mAuthorization).method("MKCOL", null).build();

    try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
    {
      int responseCode = response.code();
      if (responseCode / 100 != 2 && responseCode != 405)
        throw responseCode == 401 ? new SyncOpException.AuthExpiredException() : unexpectedHttp(responseCode);
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error trying to create directory " + dir, e);
      throw new SyncOpException.NetworkException();
    }
  }

  @Override
  public HashMap<String, String> fetchBmFilesStateMap() throws SyncOpException
  {
    RequestBody body = RequestBody.create("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                              + "<d:propfind xmlns:d=\"DAV:\" xmlns:oc=\"http://owncloud.org/ns\">"
                                              + "<d:prop><oc:checksums/></d:prop>"
                                              + "</d:propfind>",
                                          MediaType.get("application/xml; charset=utf-8"));

    Request request = new Request.Builder()
                          .url(mBmDir.toString())
                          .method("PROPFIND", body)
                          .header("Authorization", mAuthorization)
                          .header("Depth", "1")
                          .build();

    try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
    {
      throwIfUnsuccessful(response.code()); // Expected 207

      // Response format:
      // <?xml version="1.0"?>
      // <d:multistatus xmlns:d="DAV:" xmlns:s="http://sabredav.org/ns" xmlns:oc="http://owncloud.org/ns">
      //   <d:response>
      //     <d:href>/remote.php/dav/files/username/Organic Maps/bookmarks/My Places.kml</d:href>
      //     <d:propstat>
      //       <d:prop>
      //        <oc:checksums>
      //          <oc:checksum>SHA1:55ac6286e3e4f4ada5d0448333fa99fc5a891a73</oc:checksum>
      //        </oc:checksums>
      //       </d:prop>
      //       <d:status>HTTP/1.1 200 OK</d:status>
      //     </d:propstat>
      //   </d:response>
      // ... other response tags
      // </d:multistatus>

      XmlPullParser parser = Xml.newPullParser();
      parser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, true);
      parser.setInput(Objects.requireNonNull(response.body()).byteStream(), null);

      HashMap<String, String> result = new HashMap<>();
      int eventType = parser.getEventType();
      while (eventType != XmlPullParser.END_DOCUMENT)
      {
        if (eventType == XmlPullParser.START_TAG && !parser.isEmptyElementTag() && "response".equals(parser.getName())
            && XMLNS_DAV.equals(parser.getNamespace()))
        {
          String fileName = null;
          String sha1 = null;
          while (!(eventType == XmlPullParser.END_TAG && "response".equals(parser.getName())
                   && XMLNS_DAV.equals(parser.getNamespace())))
          {
            if (eventType == XmlPullParser.START_TAG && "checksum".equals(parser.getName())
                && XMLNS_OC.equals(parser.getNamespace()))
            {
              String checksum = trimQuotes(readText(parser));
              if (checksum != null && checksum.startsWith("SHA1:"))
                sha1 = checksum.substring("SHA1:".length()).trim().toLowerCase();
            }
            else if (eventType == XmlPullParser.START_TAG && "href".equals(parser.getName())
                     && XMLNS_DAV.equals(parser.getNamespace()))
            {
              String href = URLDecoder.decode(trimQuotes(readText(parser)), "UTF-8");
              String name = "";
              if (href != null)
                name = href.substring(href.lastIndexOf("/") + 1);
              if (name.endsWith(".kml")) // Non kml files are ignored as of now.
                fileName = name;
            }

            if (fileName != null)
              result.put(fileName, sha1);

            eventType = parser.next();
          }
        }
        eventType = parser.next();
      }
      Logger.i(TAG, result.size() + " kml files found in cloud dir.");

      for (Map.Entry<String, String> entry : result.entrySet())
      {
        if (entry.getValue() == null) // Should be the case for user-uploaded files
          entry.setValue(recalculateHash(entry.getKey()));
      }

      return result;
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error trying to fetch bookmark files' state.", e);
      throw new SyncOpException.NetworkException();
    }
    catch (XmlPullParserException e)
    {
      Logger.e(TAG, "Error parsing XML while trying to fetch bookmark files' state.", e);
      throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
    }
  }

  private String recalculateHash(String bmFileName) throws SyncOpException
  {
    Request request = new Request.Builder()
                          .url(getBmUri(bmFileName).toString())
                          .header("X-Recalculate-Hash", "sha1")
                          .method("PATCH", null)
                          .header("Authorization", mAuthorization)
                          .build();

    try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
    {
      String checksum = response.header("OC-Checksum", null);
      if (checksum == null)
        throw new SyncOpException.UnexpectedException("nextcloud-recalculate-hash unsupported");

      Logger.i(TAG, "Recalculated checksum: " + checksum);
      return checksum.substring("SHA1:".length()).trim().toLowerCase();
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error sending recalculate-hash request", e);
      throw new SyncOpException.NetworkException();
    }
  }

  @Override
  public void downloadBookmarkFile(String fileName, File destinationFile) throws SyncOpException
  {
    Request request = new Request.Builder()
                          .url(getBmUri(fileName).toString())
                          .header("Authorization", mAuthorization)
                          .method("GET", null)
                          .build();
    try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
    {
      throwIfUnsuccessful(response.code()); // Expected 200

      ResponseBody body = response.body();
      if (body == null)
        throw new SyncOpException.UnexpectedException("Empty download response");
      try (InputStream inputStream = body.byteStream();
           OutputStream outputStream = new FileOutputStream(destinationFile, false))
      {
        byte[] buffer = new byte[8192];
        int bytesRead;
        while ((bytesRead = inputStream.read(buffer)) != -1)
          outputStream.write(buffer, 0, bytesRead);
        outputStream.flush();
      }
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error downloading bookmark file to " + destinationFile.getPath(), e);
      throw new SyncOpException.NetworkException();
    }
  }

  private class NcEditSession extends SyncClient.EditSession implements Runnable
  {
    private static final int LOCKFILE_VALIDITY_DURATION_MS =
        20_000; // Must be the same on all devices, so should be fixed at first release.
    private static final int LOCKFILE_REFRESH_INTERVAL_MS = 15_000;

    private boolean mStopped = false;
    private final Handler mHandler;

    ///  Lockfile renewal runnable.
    @Override
    public void run()
    {
      try
      {
        if (!mStopped)
          touchLockfile();
        if (!mStopped)
          mHandler.postDelayed(this, LOCKFILE_REFRESH_INTERVAL_MS);
      }
      catch (SyncOpException e)
      {
        Logger.w(TAG, "Error renewing lockfile", e);
        // Won't be a problem at all, because other operations are going to fail for a similar cause, too.
      }
    }

    private NcEditSession() throws Syncer.LockAlreadyHeldException, SyncOpException
    {
      mHandler = new Handler(Objects.requireNonNull(Looper.myLooper()));
      long lastLocked = msSinceLastLocked();
      if (lastLocked < LOCKFILE_VALIDITY_DURATION_MS)
        throw new Syncer.LockAlreadyHeldException(LOCKFILE_VALIDITY_DURATION_MS - lastLocked);

      touchLockfile();
      mHandler.postDelayed(this, LOCKFILE_REFRESH_INTERVAL_MS);
    }

    @Override
    public void putBookmarkFile(String fileName, byte[] fileBytes, String checksum) throws SyncOpException
    {
      RequestBody body = RequestBody.create(fileBytes, MediaType.get("application/vnd.google-earth.kml+xml"));
      Request request = new Request.Builder()
                            .url(getBmUri(fileName).toString())
                            .method("PUT", body)
                            .header("OC-Checksum", "SHA1:" + checksum)
                            .header("Authorization", mAuthorization)
                            .build();
      try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
      {
        throwIfUnsuccessful(response.code());
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Error trying to upload file " + fileName + ", size " + fileBytes.length + " bytes.", e);
        throw new SyncOpException.NetworkException();
      }
    }

    @Override
    public void deleteBookmarksFile(String fileName) throws SyncOpException
    {
      try
      {
        int statusCode = deleteFile(getBmUri(fileName).toString());
        if (statusCode != 404)
          throwIfUnsuccessful(statusCode);
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Unable to delete bookmark file " + fileName, e);
        throw new SyncOpException.NetworkException();
      }
    }

    @Override
    public void close()
    {
      mStopped = true;
      mHandler.removeCallbacks(this);
      deleteLockFile();
    }

    /**
     * @return The time elapsed in milliseconds since the lockfile was last overwritten. If there's no lockfile, return
     *     2147483647 (25ish days).
     */
    private long msSinceLastLocked() throws SyncOpException
    {
      RequestBody body = RequestBody.create("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                                + "<d:propfind xmlns:d=\"DAV:\">"
                                                + "<d:prop><d:getlastmodified/></d:prop>"
                                                + "</d:propfind>",
                                            MediaType.get("application/xml; charset=utf-8"));

      Request request = new Request.Builder()
                            .url(getLockfileUri().toString())
                            .method("PROPFIND", body)
                            .header("Authorization", mAuthorization)
                            .header("Depth", "0")
                            .build();

      // Expected Response Format:
      // Date: Sun, 29 Jun 2025 19:32:20 GMT
      //
      // <?xml version="1.0"?>
      // <d:multistatus xmlns:d="DAV:" xmlns:s="http://sabredav.org/ns" xmlns:oc="http://owncloud.org/ns"
      // xmlns:nc="http://nextcloud.org/ns">
      //   <d:response>
      //     <d:href>/remote.php/files/username/Organic Maps/.lock</d:href>
      //     <d:propstat>
      //       <d:prop>
      //         <d:getlastmodified>Sun, 29 Jun 2025 13:39:40 GMT</d:getlastmodified>
      //       </d:prop>
      //       <d:status>HTTP/1.1 200 OK</d:status>
      //     </d:propstat>
      //   </d:response>
      // </d:multistatus>
      try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
      {
        int code = response.code();
        if (code == 404)
          return Integer.MAX_VALUE; // Not Long.MAX_VALUE because it won't be overflow friendly
        throwIfUnsuccessful(code);

        long currentTime;
        String dateHeader = response.header("Date", null);
        if (dateHeader != null)
          currentTime = toTimestampMs(dateHeader);
        else
          currentTime = System.currentTimeMillis();

        XmlPullParser parser = Xml.newPullParser();
        parser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, true);
        parser.setInput(Objects.requireNonNull(response.body()).byteStream(), null);

        int eventType = parser.getEventType();
        while (eventType != XmlPullParser.END_DOCUMENT)
        {
          if (eventType == XmlPullParser.START_TAG && "getlastmodified".equals(parser.getName())
              && XMLNS_DAV.equals(parser.getNamespace()))
          {
            String lastModified = trimQuotes(readText(parser));
            if (lastModified != null)
              return currentTime - toTimestampMs(lastModified);
          }
          eventType = parser.next();
        }
        Logger.e(TAG, "Unable to determine lockfile last modified time.");
        throw new SyncOpException.UnexpectedException();
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Error trying to attain lock.", e);
        throw new SyncOpException.NetworkException();
      }
      catch (XmlPullParserException e)
      {
        Logger.e(TAG, "Error parsing XML while trying to determine lockfile last modified time.", e);
        throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
      }
    }

    private void touchLockfile() throws SyncOpException
    {
      RequestBody body = RequestBody.create(new byte[] {});
      Request request = new Request.Builder()
                            .method("PUT", body)
                            .url(getLockfileUri().toString())
                            .header("Authorization", mAuthorization)
                            .build();

      try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
      {
        throwIfUnsuccessful(response.code());
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Error (over)writing the lockfile", e);
        throw new SyncOpException.NetworkException();
      }
    }

    private static long toTimestampMs(String value) throws SyncOpException.UnexpectedException
    {
      try
      {
        return ZonedDateTime.parse(value, mTimeFormatter).toInstant().toEpochMilli();
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
          return new SimpleDateFormat(value, Locale.US).parse(value).toInstant().toEpochMilli();
        }
        catch (ParseException ignored)
        {}
      }
      throw new SyncOpException.UnexpectedException("Unable to parse date " + value);
    }

    private int deleteFile(String fileUrl) throws IOException
    {
      Request request =
          new Request.Builder().url(fileUrl).method("DELETE", null).header("Authorization", mAuthorization).build();
      Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute();
      int statusCode = response.code();
      response.close();
      return statusCode;
    }

    private void deleteLockFile()
    {
      try
      {
        deleteFile(getLockfileUri().toString());
      }
      catch (IOException e)
      {
        // Inconsequential
        Logger.w(TAG, "Unable to delete lockfile", e);
      }
    }
  }

  @Override
  public EditSession getEditSession() throws Syncer.LockAlreadyHeldException, SyncOpException
  {
    return new NcEditSession();
  }

  private static SyncOpException.UnexpectedException unexpectedHttp(int statusCode)
  {
    return new SyncOpException.UnexpectedException("HTTP " + statusCode);
  }

  @Nullable
  private static String readText(XmlPullParser parser) throws IOException, XmlPullParserException
  {
    if (parser.isEmptyElementTag())
      return null; // This is a self-closing tag

    String result = "";
    if (parser.next() == XmlPullParser.TEXT)
    {
      result = parser.getText();
      parser.nextTag();
    }
    return "".equals(result) ? null : result;
  }

  private static String trimQuotes(@Nullable String string)
  {
    if (string != null && string.startsWith("\"") && string.endsWith("\""))
      return string.substring(1, string.length() - 1);
    return string;
  }

  private static void throwIfUnsuccessful(int statusCode) throws SyncOpException
  {
    if (statusCode / 100 != 2)
      throw statusCode == 401 ? new SyncOpException.AuthExpiredException() : unexpectedHttp(statusCode);
  }
}
