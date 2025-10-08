package app.organicmaps.sdk.sync.nextcloud;

import android.net.Uri;
import android.util.Base64;
import android.util.Xml;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.sync.CloudFilesState;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.sync.engine.EditSession;
import app.organicmaps.sdk.sync.engine.LockAlreadyHeldException;
import app.organicmaps.sdk.sync.engine.SyncClient;
import app.organicmaps.sdk.util.FileUtils;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URLDecoder;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import okhttp3.MediaType;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

public class NextcloudClient implements SyncClient
{
  private static final String TAG = NextcloudClient.class.getSimpleName();
  private static final String ORGANIC_MAPS_DIRECTORY = "Organic Maps";
  private static final String BOOKMARKS_SUBFOLDER = "bookmarks";
  private static final String XMLNS_OC = "http://owncloud.org/ns";
  static final String XMLNS_DAV = "DAV:";

  private final String mAuthorization;
  private final Uri mRootDir;
  private final Uri mBmDir;

  public NextcloudClient(NextcloudAuth authState)
  {
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
      ClientUtils.throwIfUnsuccessful(code);

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
        throw responseCode == 401 ? new SyncOpException.AuthExpiredException()
                                  : ClientUtils.unexpectedHttp(responseCode);
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Error trying to create directory " + dir, e);
      throw new SyncOpException.NetworkException();
    }
  }

  @Override
  public CloudFilesState fetchCloudFilesState() throws SyncOpException
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
      ClientUtils.throwIfUnsuccessful(response.code()); // Expected 207

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

      CloudFilesState result = new CloudFilesState(new HashMap<>(), new HashSet<>());
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
              // noinspection CharsetObjectCanBeUsed
              String href = URLDecoder.decode(trimQuotes(readText(parser)), "UTF-8");
              String name = "";
              if (href != null)
                name = href.substring(href.lastIndexOf("/") + 1);
              if (name.matches(SyncManager.FILENAME_REGEX))
                fileName = name;
            }

            if (fileName != null)
              result.omBookmarkFiles().put(fileName, sha1); // User uploaded files will be filtered later

            eventType = parser.next();
          }
        }
        eventType = parser.next();
      } // Parse xml response complete.

      for (Map.Entry<String, String> entry : result.omBookmarkFiles().entrySet())
      {
        if (entry.getValue() == null || !entry.getKey().endsWith(".kml")) // Should be the case for user-uploaded files
          result.userUploadedFiles().add(entry.getKey());
      }
      for (String userUploadedFile : result.userUploadedFiles())
        result.omBookmarkFiles().remove(userUploadedFile);

      Logger.i(TAG, "Found " + result.omBookmarkFiles().size() + " OM-uploaded files, and "
                        + result.userUploadedFiles().size() + " user-uploaded files in the cloud dir.");
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

  @Override
  public void downloadBookmarkFile(String fileName, File destinationFile) throws SyncOpException
  {
    Request request = new Request.Builder()
                          .url(ClientUtils.getBmUri(mBmDir, fileName).toString())
                          .header("Authorization", mAuthorization)
                          .method("GET", null)
                          .build();
    try (Response response = SyncManager.INSTANCE.getInsecureOkHttpClient().newCall(request).execute())
    {
      ClientUtils.throwIfUnsuccessful(response.code()); // Expected 200

      try (InputStream inputStream = response.body().byteStream();
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

  @Override
  public EditSession getEditSession() throws LockAlreadyHeldException, SyncOpException
  {
    // lockFileUri must not be changed across releases
    Uri lockFileUri = mRootDir.buildUpon().appendPath(".lock").build();
    return new NcEditSession(mAuthorization, lockFileUri, mBmDir);
  }

  static String trimQuotes(@Nullable String string)
  {
    if (string != null && string.startsWith("\"") && string.endsWith("\""))
      return string.substring(1, string.length() - 1);
    return string;
  }

  @Nullable
  static String readText(XmlPullParser parser) throws IOException, XmlPullParserException
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
}
