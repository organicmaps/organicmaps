package app.organicmaps.sdk.sync.nextcloud;

import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Xml;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.sync.engine.EditSession;
import app.organicmaps.sdk.sync.engine.LockAlreadyHeldException;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;
import java.util.Objects;
import okhttp3.MediaType;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

class NcEditSession implements EditSession, Runnable
{
  private static final String TAG = NcEditSession.class.getSimpleName();
  private static final int LOCKFILE_VALIDITY_DURATION_MS =
      20_000; // Must be the same on all devices, so should be fixed at first release.
  private static final int LOCKFILE_REFRESH_INTERVAL_MS = 15_000;

  private boolean mStopped = false;
  private final String mAuthorization;
  private final Uri mLockFile;
  private final Uri mBookmarksDir;
  private final HandlerThread mHandlerThread;
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

  NcEditSession(String authHeader, Uri lockFile, Uri bookmarksDir) throws LockAlreadyHeldException, SyncOpException
  {
    mAuthorization = authHeader;
    mLockFile = lockFile;
    mBookmarksDir = bookmarksDir;
    mHandlerThread = new HandlerThread("NcSync"); // used only for renewing lockfile
    mHandlerThread.start();
    mHandler = new Handler(mHandlerThread.getLooper());
    long lastLocked = msSinceLastLocked();
    if (lastLocked < LOCKFILE_VALIDITY_DURATION_MS)
      throw new LockAlreadyHeldException(LOCKFILE_VALIDITY_DURATION_MS - lastLocked);

    touchLockfile();
    mHandler.postDelayed(this, LOCKFILE_REFRESH_INTERVAL_MS);
  }

  @Override
  public void putBookmarkFile(String fileName, byte[] fileBytes, String checksum) throws SyncOpException
  {
    RequestBody body = RequestBody.create(fileBytes, MediaType.get(SyncManager.KML_MIME_TYPE));
    Request request = new Request.Builder()
                          .url(ClientUtils.getBmUri(mBookmarksDir, fileName).toString())
                          .method("PUT", body)
                          .header("OC-Checksum", "SHA1:" + checksum)
                          .header("Authorization", mAuthorization)
                          .build();
    try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
    {
      ClientUtils.throwIfUnsuccessful(response.code());
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
      int statusCode = deleteFile(ClientUtils.getBmUri(mBookmarksDir, fileName).toString());
      if (statusCode != 404)
        ClientUtils.throwIfUnsuccessful(statusCode);
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
    mHandlerThread.quit();
    try
    {
      mHandlerThread.join();
    }
    catch (InterruptedException ignored)
    {}
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
                          .url(mLockFile.toString())
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
      ClientUtils.throwIfUnsuccessful(code);

      long currentTime;
      String dateHeader = response.header("Date", null);
      if (dateHeader != null)
        currentTime = ClientUtils.toTimestampMs(dateHeader);
      else
        currentTime = System.currentTimeMillis();

      XmlPullParser parser = Xml.newPullParser();
      parser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, true);
      parser.setInput(Objects.requireNonNull(response.body()).byteStream(), null);

      int eventType = parser.getEventType();
      while (eventType != XmlPullParser.END_DOCUMENT)
      {
        if (eventType == XmlPullParser.START_TAG && "getlastmodified".equals(parser.getName())
            && NextcloudClient.XMLNS_DAV.equals(parser.getNamespace()))
        {
          String lastModified = NextcloudClient.trimQuotes(NextcloudClient.readText(parser));
          if (lastModified != null)
            return currentTime - ClientUtils.toTimestampMs(lastModified);
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
                          .url(mLockFile.toString())
                          .header("Authorization", mAuthorization)
                          .build();

    try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
    {
      ClientUtils.throwIfUnsuccessful(response.code());
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error (over)writing the lockfile", e);
      throw new SyncOpException.NetworkException();
    }
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
      deleteFile(mLockFile.toString());
    }
    catch (IOException e)
    {
      // Inconsequential
      Logger.w(TAG, "Unable to delete lockfile", e);
    }
  }
}
