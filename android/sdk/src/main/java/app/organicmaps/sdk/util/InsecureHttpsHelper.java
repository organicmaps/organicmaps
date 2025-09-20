package app.organicmaps.sdk.util;

import android.annotation.SuppressLint;
import app.organicmaps.sdk.util.log.Logger;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import javax.net.ssl.*;
import okhttp3.OkHttpClient;

/**
 * An HttpUrlConnection alternative that allows insecure (e.g. self-signed) certificates.
 */
public class InsecureHttpsHelper
{
  private static final String TAG = InsecureHttpsHelper.class.getSimpleName();

  @SuppressLint("CustomX509TrustManager")
  private static final TrustManager[] INSECURE_TRUST_MANAGERS = new TrustManager[]
      {
        new X509TrustManager(){
          public X509Certificate[] getAcceptedIssuers()
          {
            return new X509Certificate[] {};
          }
          @SuppressLint("TrustAllX509TrustManager")
          public void checkClientTrusted(X509Certificate[] certs, String authType) {}
          @SuppressLint("TrustAllX509TrustManager")
          public void checkServerTrusted(X509Certificate[] certs, String authType) {}
        }
      };

  private static final HostnameVerifier ALLOW_ALL_HOSTNAMES = (hostname, session) -> true;

  private static SSLContext createInsecureSSLContext() throws NoSuchAlgorithmException, KeyManagementException
  {
    SSLContext sslContext = SSLContext.getInstance("TLS");
    sslContext.init(null, INSECURE_TRUST_MANAGERS, new SecureRandom());
    return sslContext;
  }

  /**
   * Open a connection to the specified URL that bypasses certificate validation
   *
   * @param urlString URL to connect to
   * @return HttpURLConnection with disabled SSL certificate verification
   * @throws IOException if connection fails
   */
  public static HttpURLConnection openInsecureConnection(String urlString) throws IOException
  {
    URL url = new URL(urlString);
    if (url.getProtocol().equalsIgnoreCase("https"))
    {
      try
      {
        HttpsURLConnection connection = (HttpsURLConnection) url.openConnection();
        connection.setSSLSocketFactory(createInsecureSSLContext().getSocketFactory());
        connection.setHostnameVerifier(ALLOW_ALL_HOSTNAMES);
        return connection;
      }
      catch (NoSuchAlgorithmException | KeyManagementException e)
      {
        Logger.w(TAG, "Unable to open an insecure HttpsURLConnection. Falling back to HttpURLConnection.", e);
        return (HttpURLConnection) url.openConnection();
      }
    }
    else
      return (HttpURLConnection) url.openConnection();
  }

  /**
   * @return an insecure OkHttpClient that bypasses certificate validation
   */
  public static OkHttpClient createInsecureOkHttpClient()
  {
    OkHttpClient.Builder newBuilder = new OkHttpClient.Builder();
    try
    {
      newBuilder.sslSocketFactory(createInsecureSSLContext().getSocketFactory(),
          (X509TrustManager) INSECURE_TRUST_MANAGERS[0]);
      newBuilder.hostnameVerifier((hostname, session) -> true);
    }
    catch (NoSuchAlgorithmException | KeyManagementException e)
    {
      Logger.w(TAG, "Unable to create an insecure OkHttp client. Falling back to default client", e);
    }
    return newBuilder.build();
  }
}
