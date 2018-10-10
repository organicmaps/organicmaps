package com.mapswithme.util;

import android.content.Context;
import android.net.SSLCertificateSocketFactory;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.RawRes;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mopub.common.Preconditions;
import com.mopub.common.VisibleForTesting;
import com.mopub.common.logging.MoPubLog;
import com.mopub.common.util.Reflection;
import com.mopub.network.InetAddressUtils;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.UnrecoverableKeyException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLHandshakeException;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;

/**
 * An {@link javax.net.ssl.SSLSocketFactory} that supports TLS settings for the MoPub ad servers.
 */
public class TLSSocketFactory extends SSLSocketFactory {

  @Nullable private SSLSocketFactory mCertificateSocketFactory;

  protected SSLContext sslContext = SSLContext.getInstance("TLS");

  public TLSSocketFactory(KeyStore keyStore) throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException
  {
    KeyManagerFactory kmf = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
    kmf.init(keyStore, "".toCharArray());
    sslContext.init(kmf.getKeyManagers(), new TrustManager[]{ new AdditionalKeyStoresSSLSocketFactory.ClientKeyStoresTrustManager(keyStore)}, new SecureRandom());
  }

  @NonNull
  public static TLSSocketFactory getDefault(KeyStore keyStore) throws UnrecoverableKeyException,
                                                                      NoSuchAlgorithmException, KeyStoreException, KeyManagementException
  {
    TLSSocketFactory factory = new TLSSocketFactory(keyStore);
    factory.mCertificateSocketFactory = SSLCertificateSocketFactory.getDefault((int) TimeUnit.MINUTES.toMillis(5), null);

    return factory;
  }

  // Forward all methods. Enable TLS 1.1 and 1.2 before returning.

  // SocketFactory overrides
  @Override
  public Socket createSocket() throws IOException {
    if (mCertificateSocketFactory == null) {
      throw new SocketException("SSLSocketFactory was null. Unable to create socket.");
    }
    final Socket socket = mCertificateSocketFactory.createSocket();
    enableTlsIfAvailable(socket);
    return socket;
  }

  @Override
  public Socket createSocket(final String host, final int i) throws IOException, UnknownHostException {
    if (mCertificateSocketFactory == null) {
      throw new SocketException("SSLSocketFactory was null. Unable to create socket.");
    }
    final Socket socket = mCertificateSocketFactory.createSocket(host, i);
    enableTlsIfAvailable(socket);
    return socket;
  }

  @Override
  public Socket createSocket(final String host, final int port, final InetAddress localhost, final int localPort) throws IOException, UnknownHostException {
    if (mCertificateSocketFactory == null) {
      throw new SocketException("SSLSocketFactory was null. Unable to create socket.");
    }
    final Socket socket = mCertificateSocketFactory.createSocket(host, port, localhost, localPort);
    enableTlsIfAvailable(socket);
    return socket;
  }

  @Override
  public Socket createSocket(final InetAddress address, final int port) throws IOException {
    if (mCertificateSocketFactory == null) {
      throw new SocketException("SSLSocketFactory was null. Unable to create socket.");
    }
    final Socket socket = mCertificateSocketFactory.createSocket(address, port);
    enableTlsIfAvailable(socket);
    return socket;
  }

  @Override
  public Socket createSocket(final InetAddress address, final int port, final InetAddress localhost, final int localPort) throws IOException {
    if (mCertificateSocketFactory == null) {
      throw new SocketException("SSLSocketFactory was null. Unable to create socket.");
    }
    final Socket socket = mCertificateSocketFactory.createSocket(address, port, localhost, localPort);
    enableTlsIfAvailable(socket);
    return socket;
  }

  // SSLSocketFactory overrides

  @Override
  public String[] getDefaultCipherSuites() {
    if (mCertificateSocketFactory == null) {
      return new String[]{};
    }
    return mCertificateSocketFactory.getDefaultCipherSuites();
  }

  @Override
  public String[] getSupportedCipherSuites() {
    if (mCertificateSocketFactory == null) {
      return new String[]{};
    }
    return mCertificateSocketFactory.getSupportedCipherSuites();
  }

  @Override
  public Socket createSocket(final Socket socketParam, final String host, final int port, final boolean autoClose) throws IOException {
    if (mCertificateSocketFactory == null) {
      throw new SocketException("SSLSocketFactory was null. Unable to create socket.");
    }

    // There is a bug in Android before version 6.0 where SNI does not work, so we try to do
    // it manually here.
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
      // Don't use the original socket and create a new one. This closes the original socket
      // if the autoClose flag is set.
      if (autoClose && socketParam != null) {
        socketParam.close();
      }

      final Socket socket = mCertificateSocketFactory.createSocket(
          InetAddressUtils.getInetAddressByName(host), port);
      enableTlsIfAvailable(socket);
      doManualServerNameIdentification(socket, host);
      return socket;
    }

    final Socket socket = mCertificateSocketFactory.createSocket(socketParam, host, port,
                                                                 autoClose);
    enableTlsIfAvailable(socket);
    return socket;
  }

  /**
   * Some versions of Android fail to do server name identification (SNI) even though they are
   * able to. This method forces SNI to happen, if possible. SNI is only used in https
   * connections, and this method will no-op for http connections. This method throws an
   * SSLHandshakeException if SNI fails. This method may also throw other socket-related
   * IOExceptions.
   *
   * @param socket The socket to do SNI on
   * @param host   The host to verify the server name
   * @throws IOException
   */
  private void doManualServerNameIdentification(@NonNull final Socket socket,
                                                @Nullable final String host) throws IOException {
    Preconditions.checkNotNull(socket);

    if (mCertificateSocketFactory == null) {
      throw new SocketException("SSLSocketFactory was null. Unable to create socket.");
    }

    if (socket instanceof SSLSocket) {
      final SSLSocket sslSocket = (SSLSocket) socket;
      setHostnameOnSocket((SSLCertificateSocketFactory) mCertificateSocketFactory, sslSocket,
                          host);
      verifyServerName(sslSocket, host);
    }
  }

  /**
   * Calling setHostname on a socket turns on the server name identification feature.
   * Unfortunately, this was introduced in Android version 17, so we do what we can.
   */
  static void setHostnameOnSocket(@NonNull final SSLCertificateSocketFactory certificateSocketFactory,
                                  @NonNull final SSLSocket sslSocket, @Nullable final String host) {
    Preconditions.checkNotNull(certificateSocketFactory);
    Preconditions.checkNotNull(sslSocket);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      certificateSocketFactory.setHostname(sslSocket, host);
    } else {
      try {
        new Reflection.MethodBuilder(sslSocket, "setHostname")
            .addParam(String.class, host)
            .execute();
      } catch (Exception e) {

      }
    }
  }

  static void verifyServerName(@NonNull final SSLSocket sslSocket,
                               @Nullable final String host) throws IOException {
    Preconditions.checkNotNull(sslSocket);

    sslSocket.startHandshake();
    final HostnameVerifier hostnameVerifier = HttpsURLConnection.getDefaultHostnameVerifier();
    if (!hostnameVerifier.verify(host, sslSocket.getSession())) {
      throw new SSLHandshakeException("Server Name Identification failed.");
    }
  }

  private void enableTlsIfAvailable(@Nullable Socket socket) {
    if (socket instanceof SSLSocket) {
      SSLSocket sslSocket = (SSLSocket) socket;
      String[] supportedProtocols = sslSocket.getSupportedProtocols();
      // Make sure all supported protocols are enabled. Android does not enable TLSv1.1 or
      // TLSv1.2 by default.
      sslSocket.setEnabledProtocols(supportedProtocols);
    }
  }

  public static SSLSocketFactory create()
  {
    InputStream stream = MwmApplication.get().getResources().openRawResource(R.raw.my_cret);
    String type = KeyStore.getDefaultType();
    try
    {
      KeyStore keyStore = KeyStore.getInstance("PKCS12");
      keyStore.load(stream, "123".toCharArray());
      String algorithm = TrustManagerFactory.getDefaultAlgorithm();
      TrustManagerFactory trustManagerFactory = TrustManagerFactory.getInstance(algorithm);
      trustManagerFactory.init(keyStore);
      SSLContext sslContext = SSLContext.getInstance("TLS");
      sslContext.init(null, trustManagerFactory.getTrustManagers(), new SecureRandom());
      return sslContext.getSocketFactory();
    }
    catch (KeyStoreException e)
    {
      e.printStackTrace();
    }
    catch (CertificateException e)
    {
      e.printStackTrace();
    }
    catch (NoSuchAlgorithmException e)
    {
      e.printStackTrace();
    }
    catch (IOException e)
    {
      e.printStackTrace();
    }
    catch (KeyManagementException e)
    {
      e.printStackTrace();
    }
    throw new RuntimeException("e");
  }


  public static SSLSocketFactory createFactory()
  {
    InputStream caInput = null;
    try {

      KeyStore keystore = KeyStore.getInstance("PKCS12");

      keystore.load(MwmApplication.get().getResources().openRawResource(R.raw.cert), "".toCharArray());

      return new AdditionalKeyStoresSSLSocketFactory(keystore);


    } catch (Exception ex) {
      throw new RuntimeException(ex);

    } finally {
      if (caInput != null) {
        try {
          caInput.close();
        } catch (IOException ignored) {
        }
      }
    }
  }

  public static SSLSocketFactory create(Context context, @RawRes int caRawFile) {
    InputStream caInput = null;
    try {
      // Generate the CA Certificate from the raw resource file
      caInput = context.getResources().openRawResource(caRawFile);
      Certificate ca = CertificateFactory.getInstance("X.509", "BC").generateCertificate(caInput);

      // Load the key store using the CA
      KeyStore keyStore = KeyStore.getInstance(KeyStore.getDefaultType());
      keyStore.load(null, null);
      keyStore.setCertificateEntry("ca", ca);

      // Initialize the TrustManager with this CA
      TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
      tmf.init(keyStore);

      /*/*    InputStream stream = MwmApplication.get().getResources().openRawResource(R.raw.cert);
    String type = KeyStore.getDefaultType();
    try
    {
      KeyStore keyStore = KeyStore.getInstance(type);
      keyStore.load(stream, null);
      String algorithm = TrustManagerFactory.getDefaultAlgorithm();
      TrustManagerFactory trustManagerFactory = TrustManagerFactory.getInstance(algorithm);
      trustManagerFactory.init(keyStore);
    }*/
      // Create an SSL context that uses the created trust manager
      SSLContext sslContext = SSLContext.getInstance("TLS");
      sslContext.init(null, tmf.getTrustManagers(), new SecureRandom());
      return sslContext.getSocketFactory();

    } catch (Exception ex) {
      throw new RuntimeException(ex);

    } finally {
      if (caInput != null) {
        try {
          caInput.close();
        } catch (IOException ignored) {
        }
      }
    }
  }
}
