package com.mapswithme.maps.location;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.log.DebugLogger;
import com.mapswithme.util.log.Logger;

import javax.net.SocketFactory;
import javax.net.ssl.SSLSocketFactory;
import java.io.IOException;
import java.net.Socket;
import java.net.SocketException;

/**
 * Implements {@link PlatformSocket} interface that will be used by the core for
 * sending/receiving the raw data trough platform socket interface.
 * <p>
 * The instance of this class is supposed to be created in JNI layer
 * and supposed to be used in the thread safe environment, i.e. thread safety
 * should be provided externally (by the client of this class).
 * <p>
 * <b>All public methods are blocking and shouldn't be called from the main thread.</b>
 */
class SocketWrapper implements PlatformSocket
{
  private final static Logger sLogger = new DebugLogger(SocketWrapper.class.getSimpleName());
  @Nullable
  private Socket mSocket;
  @Nullable
  private String mHost;
  private int mPort;

  @Override
  public boolean open(@NonNull String host, int port)
  {
    if (mSocket != null)
    {
      sLogger.e("Socket is already opened. Seems that it wasn't be closed.");
      return false;
    }

    mHost = host;
    mPort = port;

    Socket socket = createSocket(host, port, true);
    if (socket != null && socket.isConnected())
      mSocket = socket;

    return mSocket != null;
  }

  @Nullable
  private static Socket createSocket(@NonNull String host, int port, boolean ssl)
  {
    if (ssl)
    {
      SocketFactory sf = SSLSocketFactory.getDefault();
      try
      {
        return sf.createSocket(host, port);
      } catch (IOException e)
      {
        sLogger.e("Failed to create the ssl socket, mHost", host, " mPort = ", port, e);
      }
    } else
    {
      try
      {
        return new Socket(host, port);
      } catch (IOException e)
      {
        sLogger.e("Failed to create the socket, mHost = ", host, " mPort = ", port, e);
      }
    }
    return null;
  }

  @Override
  public boolean close()
  {
    if (mSocket == null)
    {
      sLogger.e("Socket is already closed or it wasn't be opened before");
      return false;
    }

    try
    {
      mSocket.close();
      return true;
    } catch (IOException e)
    {
      sLogger.e("Failed to close socket: ", this, e);
    } finally
    {
      mSocket = null;
    }
    return false;
  }

  @Override
  public boolean read(@NonNull byte[] data, int count)
  {
    if (mSocket == null)
      throw new IllegalStateException("Socket must be opened before reading");

    try
    {
      int read = mSocket.getInputStream().read(data);
      sLogger.e("Read ", read, " bytes in buffer with length = ", data.length);
      return true;
    } catch (IOException e)
    {
      sLogger.e("Failed to read data from socket: ", this);
    }
    return false;
  }

  @Override
  public boolean write(@NonNull byte[] data, int count)
  {
    if (mSocket == null)
      throw new IllegalStateException("Socket must be opened before writing");

    try
    {
      mSocket.getOutputStream().write(data);
      return true;
    } catch (IOException e)
    {
      sLogger.e("Failed to write data to socket: ", this);
    }
    return false;
  }

  @Override
  public void setTimeout(int millis)
  {
    if (mSocket == null)
      throw new IllegalStateException("Socket must be initialized before setting the timeout");

    try
    {
      mSocket.setSoTimeout(millis);
    } catch (SocketException e)
    {
      sLogger.e("Failed to set socket timeout: ", millis, "ms, ", this, e);
    }
  }

  @Override
  public String toString()
  {
    return "SocketWrapper{" +
           "mSocket=" + mSocket +
           ", mHost='" + mHost + '\'' +
           ", mPort=" + mPort +
           '}';
  }
}