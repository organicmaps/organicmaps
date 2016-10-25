package com.mapswithme.maps.location;

import android.net.SSLCertificateSocketFactory;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.log.DebugLogger;
import com.mapswithme.util.log.Logger;

import javax.net.ssl.SSLSocketFactory;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;

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
  private final static int DEFAULT_TIMEOUT = 30 * 1000;
  @Nullable
  private Socket mSocket;
  @Nullable
  private String mHost;
  private int mPort;
  private int mTimeout = DEFAULT_TIMEOUT;

  @Override
  public boolean open(@NonNull String host, int port)
  {
    if (mSocket != null)
    {
      sLogger.e("Socket is already opened. Seems that it wasn't closed.");
      return false;
    }

    if (!isPortAllowed(port))
    {
      sLogger.e("A wrong port number, it must be within (0-65535) range", port);
      return false;
    }

    mHost = host;
    mPort = port;

    Socket socket = createSocket(host, port, true);
    if (socket != null && socket.isConnected())
    {
      setReadSocketTimeout(socket, mTimeout);
      mSocket = socket;
    }

    return mSocket != null;
  }

  private static boolean isPortAllowed(int port)
  {
    return port >= 0 && port <= 65535;
  }

  @Nullable
  private static Socket createSocket(@NonNull String host, int port, boolean ssl)
  {
    if (ssl)
    {
      //TODO: use a boolean flag about using the secure or insecure ssl socket (must be done before release!)
      //https://jira.mail.ru/browse/MAPSME-2786
      SSLSocketFactory sf = SSLCertificateSocketFactory.getInsecure(0, null);
      try
      {
        return sf.createSocket(host, port);
      } catch (IOException e)
      {
        sLogger.e("Failed to create the ssl socket, mHost = ", host, " mPort = ", port, e);
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
  public void close()
  {
    if (mSocket == null)
    {
      sLogger.d("Socket is already closed or it wasn't opened yet");
      return;
    }

    try
    {
      mSocket.close();
    } catch (IOException e)
    {
      sLogger.e("Failed to close socket: ", this, e);
    } finally
    {
      mSocket = null;
    }
  }

  @Override
  public boolean read(@NonNull byte[] data, int count)
  {
    if (!checkSocketAndArguments(data, count))
      return false;

    sLogger.d("Reading has started, data.length = " + data.length, ", count = " + count);
    long startTime = System.nanoTime();
    int readBytes = 0;
    try
    {
      if (mSocket == null)
        throw new AssertionError("mSocket cannot be null");

      InputStream in = mSocket.getInputStream();
      while (readBytes != count && (System.nanoTime() - startTime) < mTimeout)
      {
        try
        {
          sLogger.d("Attempting to read ", count, " bytes from offset = ", readBytes);
          int read = in.read(data, readBytes, count - readBytes);

          if (read == -1)
          {
            sLogger.d("All data have been read from the stream, read bytes count = ", readBytes);
            break;
          }

          if (read == 0)
          {
            sLogger.e("0 bytes have been obtained. It's considered as error");
            break;
          }

          sLogger.d("Read bytes count = ", read);
          readBytes += read;
        } catch (SocketTimeoutException e)
        {
          long readingTime = System.nanoTime() - startTime;
          sLogger.e(e, "Socked timeout has occurred after ", readingTime, " (ms) ");
          if (readingTime > mTimeout)
          {
            sLogger.e("Socket wrapper timeout has occurred, requested count = ",
                      count - readBytes, ", " + "readBytes = ", readBytes);
            break;
          }
        }
      }
    } catch (IOException e)
    {
      sLogger.e(e, "Failed to read data from socket: ", this);
    }

    return count == readBytes;
  }

  @Override
  public boolean write(@NonNull byte[] data, int count)
  {
    if (!checkSocketAndArguments(data, count))
      return false;

    sLogger.d("Writing method has started, data.length = " + data.length, ", count = " + count);
    long startTime = System.nanoTime();
    try
    {
      if (mSocket == null)
        throw new AssertionError("mSocket cannot be null");

      OutputStream out = mSocket.getOutputStream();
      out.write(data, 0, count);
      return true;
    } catch (SocketTimeoutException e)
    {
      long writingTime = System.nanoTime() - startTime;
      sLogger.e(e, "Socked timeout has occurred after ", writingTime, " (ms) ");
    } catch (IOException e)
    {
      sLogger.e(e, "Failed to write data to socket: ", this);
    }

    return false;
  }

  private boolean checkSocketAndArguments(@NonNull byte[] data, int count)
  {
    if (mSocket == null)
    {
      sLogger.e("Socket must be opened before reading/writing");
      return false;
    }

    if (data.length < 0 || count < 0 || count > data.length)
    {
      sLogger.e("Illegal arguments, data.length = ", data.length, ", count = " + count);
      return false;
    }

    return true;
  }

  @Override
  public void setTimeout(int millis)
  {
    mTimeout = millis;
    sLogger.d("Setting the socket wrapper timeout = ", millis, " ms");
  }

  private void setReadSocketTimeout(@NonNull Socket socket, int millis)
  {
    try
    {
      socket.setSoTimeout(millis);
    } catch (SocketException e)
    {
      sLogger.e("Failed to set system socket timeout: ", millis, "ms, ", this, e);
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
