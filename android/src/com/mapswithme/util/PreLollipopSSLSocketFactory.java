package com.mapswithme.util;

import androidx.annotation.NonNull;

import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;


class PreLollipopSSLSocketFactory extends SSLSocketFactory
{
  @NonNull
  private final SSLSocketFactory mSslSocketFactory;

  PreLollipopSSLSocketFactory(@NonNull SSLSocketFactory sslSocketFactory)
  {
    super();
    this.mSslSocketFactory = sslSocketFactory;
  }

  @Override
  public String[] getDefaultCipherSuites()
  {
    return mSslSocketFactory.getDefaultCipherSuites();
  }

  @Override
  public String[] getSupportedCipherSuites()
  {
    return mSslSocketFactory.getSupportedCipherSuites();
  }

  @Override
  public SSLSocket createSocket(Socket s, String host, int port, boolean autoClose) throws IOException
  {
    SSLSocket socket = (SSLSocket) mSslSocketFactory.createSocket(s, host, port, autoClose);
    socket.setEnabledProtocols(new String[] { "TLSv1.2" });
    return socket;
  }

  @Override
  public Socket createSocket(String host, int port) throws IOException
  {
    SSLSocket socket = (SSLSocket) mSslSocketFactory.createSocket(host, port);
    socket.setEnabledProtocols(new String[] { "TLSv1.2" });
    return socket;
  }

  @Override
  public Socket createSocket(String host, int port, InetAddress localHost, int localPort) throws IOException
  {
    SSLSocket socket = (SSLSocket) mSslSocketFactory.createSocket(host, port, localHost, localPort);
    socket.setEnabledProtocols(new String[] { "TLSv1.2" });
    return socket;
  }

  @Override
  public Socket createSocket(InetAddress host, int port) throws IOException
  {
    SSLSocket socket = (SSLSocket) mSslSocketFactory.createSocket(host, port);
    socket.setEnabledProtocols(new String[] { "TLSv1.2" });
    return socket;
  }

  @Override
  public Socket createSocket(InetAddress address, int port, InetAddress localAddress, int localPort)
      throws IOException
  {
    SSLSocket socket = (SSLSocket) mSslSocketFactory.createSocket(address, port, localAddress, localPort);
    socket.setEnabledProtocols(new String[] { "TLSv1.2" });
    return socket;
  }
}
