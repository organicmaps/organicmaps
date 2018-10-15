package com.mapswithme.util;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.KeyStore;
import java.security.SecureRandom;

public class ClientCertTLSSocketFactory {

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.NETWORK);
  private static final String TAG = ClientCertTLSSocketFactory.class.getSimpleName();

  private static final String PROTOCOL = "TLS";
  private static final String ALGORITHM = "X509";
  private static final String KEY_STORE_TYPE = "PKCS12";

  public static SSLSocketFactory create(@NonNull byte[] payload, @Nullable char[] password)
  {
    InputStream inputStream = null;
    try
    {
      inputStream = new ByteArrayInputStream(payload);
      KeyStore keyStore = KeyStore.getInstance(KEY_STORE_TYPE);
      keyStore.load(inputStream, password);
      KeyManagerFactory kmf = KeyManagerFactory.getInstance(ALGORITHM);
      kmf.init(keyStore, null);
      SSLContext sslContext = SSLContext.getInstance(PROTOCOL);
      sslContext.init(kmf.getKeyManagers(), null, new SecureRandom());
      return sslContext.getSocketFactory();
    }
    catch (Exception ex)
    {
      throw new RuntimeException(ex);
    }
    finally
    {
      if (inputStream != null)
      {
        try
        {
          inputStream.close();
        }
        catch (IOException e)
        {
          LOGGER.d(TAG, "Stream not closed", e);
        }
      }
    }
  }
}
