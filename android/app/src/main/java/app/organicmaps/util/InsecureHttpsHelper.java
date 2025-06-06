package app.organicmaps.util;

import android.annotation.SuppressLint;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import javax.net.ssl.*;

/**
 * An HttpUrlConnection alternative that allows insecure (e.g. self-signed) certificates.
 */
public class InsecureHttpsHelper
{
  @SuppressLint("CustomX509TrustManager")
  private static final TrustManager[] INSECURE_TRUST_MANAGERS = new TrustManager[] {
      new X509TrustManager(){public X509Certificate[] getAcceptedIssuers(){return new X509Certificate[] {};
}

public void checkClientTrusted(X509Certificate[] certs, String authType) {}

@SuppressLint("TrustAllX509TrustManager")
public void checkServerTrusted(X509Certificate[] certs, String authType)
{}
}
}
;

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
 * @throws IOException              if connection fails
 * @throws NoSuchAlgorithmException if the TLS algorithm is not available
 * @throws KeyManagementException   if initialization fails
 */
public static HttpURLConnection openInsecureConnection(String urlString)
    throws IOException, NoSuchAlgorithmException, KeyManagementException
{
  URL url = new URL(urlString);

  if (url.getProtocol().equalsIgnoreCase("https"))
  {
    HttpsURLConnection connection = (HttpsURLConnection) url.openConnection();
    connection.setSSLSocketFactory(createInsecureSSLContext().getSocketFactory());
    connection.setHostnameVerifier(ALLOW_ALL_HOSTNAMES);
    return connection;
  }
  else
  {
    return (HttpURLConnection) url.openConnection();
  }
}
}
