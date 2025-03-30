package app.organicmaps.sync.nc;

import androidx.annotation.NonNull;

import com.thegrizzlylabs.sardineandroid.DavResource;
import com.thegrizzlylabs.sardineandroid.Sardine;
import com.thegrizzlylabs.sardineandroid.impl.OkHttpSardine;


import experiment.InsecureHttpsHelper;
import okhttp3.OkHttpClient;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

class DavClient
{
  private static final String TAG = DavClient.class.getSimpleName();
  private static final String KML_MIME_TYPE = "application/vnd.google-earth.kml+xml";
  private static final String BASE_DIR_PATH = "/OM_bookmarks_experimental/"; // to make it deeper, make sure to make the createBaseDir implementation recursive

  private final OkHttpClient mHttpClient;
  private final Sardine mSardine;
  private final String remoteBaseUrl;

  public DavClient(@NonNull String username, @NonNull String password, @NonNull String remoteServerUrl)
  {
    mHttpClient = InsecureHttpsHelper.createInsecureOkHttpClient();
    mSardine = new OkHttpSardine(mHttpClient);
    mSardine.setCredentials(username, password, true); // use preemptive because nextcloud requires auth anyways, so as to avoid duplicate requests
    this.remoteBaseUrl = remoteServerUrl + "/remote.php/dav/files/" + username + BASE_DIR_PATH;
  }

  public void attainLock()
  {
    // TODO implement
  }

  public void releaseLock()
  {
    // TODO implement
  }

  public String getRemoteUrl(File localFile)
  {
    return getRemoteUrl(localFile.getName());
  }

  public String getRemoteUrl(String fileName)
  {
    return remoteBaseUrl + fileName;
  }

  public String getRemoteBaseUrl()
  {
    return remoteBaseUrl;
  }

  public String getBaseDirETag() throws IOException
  {
    return mSardine.list(remoteBaseUrl, 0).get(0).getEtag();
  }

  public String getFileETag(String remoteUrl) throws IOException
  {
    return mSardine.list(remoteUrl).get(0).getEtag();
  }

  public void createBaseDir() throws IOException
  {
    mSardine.createDirectory(remoteBaseUrl);
  }

  public void uploadFileToRemoteURL(File localFile, String remoteUrl) throws IOException
  {
    mSardine.put(remoteUrl, localFile, KML_MIME_TYPE);
  }

  public void deleteFileAtRemoteURL(String remoteUrl) throws IOException
  {
    mSardine.delete(remoteUrl);
  }

  public InputStream downloadFile(String remoteUrl) throws IOException
  {
    return mSardine.get(remoteUrl);
  }

  /**
   * @return a list of FileMetadata corresponding to each file present inside the base bookmarks directory
   */
  public List<FileMetadata> listAllFiles() throws Exception
  {
    return mSardine.list(remoteBaseUrl, 1).stream()
      .skip(1)
      .map(davResource ->
        new FileMetadata(
          davResource.getName(),
          davResource.getEtag()
        )
      )
      .collect(Collectors.toList());
  }

  public static record FileMetadata(String name, String eTag) {}

/*
 UNNEEDED because 1 http-request can be saved by instead getting ETag and watching out for 404
  public boolean resourceExists(String remoteUrl) throws IOException
  {
    return mSardine.exists(remoteUrl);
  }
*/
}
