package app.organicmaps.sdk.downloader;

import android.annotation.TargetApi;
import android.content.Context;
import app.organicmaps.sdk.R;
import app.organicmaps.sdk.util.log.Logger;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManagerFactory;

// Fix missing root certificates for HTTPS connections on Android 7 and below:
// https://community.letsencrypt.org/t/letsencrypt-certificates-fails-on-android-phones-running-android-7-or-older/205686
@TargetApi(24)
public class Android7RootCertificateWorkaround
{
  private static final String TAG = Android7RootCertificateWorkaround.class.getSimpleName();

  @TargetApi(24)
  private static SSLSocketFactory mSslSocketFactory;

  public static void applyFixIfNeeded(HttpURLConnection connection)
  {
    // Deliberately not checking for null to have an exception from setSSLSocketFactory.
    if (android.os.Build.VERSION.SDK_INT <= android.os.Build.VERSION_CODES.N
        && connection.getURL().getProtocol().equals("https"))
      ((HttpsURLConnection) connection).setSSLSocketFactory(mSslSocketFactory);
  }

  public static void initializeIfNeeded(Context context)
  {
    if (android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.N)
      return;

    final int[] certificates = new int[] {R.raw.isrgrootx1, R.raw.globalsignr4, R.raw.gtsrootr1,
                                          R.raw.gtsrootr2,  R.raw.gtsrootr3,    R.raw.gtsrootr4};

    try
    {
      final KeyStore keyStore = KeyStore.getInstance(KeyStore.getDefaultType());
      keyStore.load(null, null);

      // Load PEM certificates from raw resources.
      for (final int rawCertificateId : certificates)
      {
        try (final InputStream caInput = context.getResources().openRawResource(rawCertificateId))
        {
          final CertificateFactory cf = CertificateFactory.getInstance("X.509");
          final Certificate ca = cf.generateCertificate(caInput);
          keyStore.setCertificateEntry("ca" + rawCertificateId, ca);
        }
      }

      final TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
      tmf.init(keyStore);

      final SSLContext sslContext = SSLContext.getInstance("TLS");
      sslContext.init(null, tmf.getTrustManagers(), null);
      mSslSocketFactory = sslContext.getSocketFactory();
    }
    catch (Exception e)
    {
      e.printStackTrace();
      Logger.e(TAG, "Failed to load certificates: " + e.getMessage());
    }
  }
}
