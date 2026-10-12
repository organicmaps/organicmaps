package app.organicmaps.sdk.location;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * The Bluetooth SPP connection to an ELM327 OBD-II adapter for the navigation without GPS in the core: the core
 * talks to the adapter, this class only connects, writes and passes the received bytes back on the UI thread.
 */
class Elm327Transport
{
  private static final String TAG = Elm327Transport.class.getSimpleName();
  private static final UUID SPP_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

  private final Handler mMainHandler = new Handler(Looper.getMainLooper());
  // Writes may block, they go one after another on their own thread.
  private final ExecutorService mWriter = Executors.newSingleThreadExecutor();
  @Nullable
  private volatile BluetoothSocket mSocket;
  // The connection the callbacks are for: the ones of a closed connection are dropped. Changed on the UI thread.
  private volatile int mConnection;

  @UiThread
  void connect(@NonNull String address)
  {
    close();
    final int connection = ++mConnection;
    new Thread(() -> run(address, connection), "Elm327").start();
  }

  @UiThread
  void write(@NonNull byte[] data)
  {
    final BluetoothSocket socket = mSocket;
    if (socket == null)
      return;
    mWriter.execute(() -> {
      try
      {
        socket.getOutputStream().write(data);
        socket.getOutputStream().flush();
      }
      catch (IOException e)
      {
        // The reader notices the broken connection.
        Logger.w(TAG, "Write failed: " + e.getMessage());
      }
    });
  }

  @UiThread
  void close()
  {
    ++mConnection;
    closeSocket();
  }

  private void run(@NonNull String address, int connection)
  {
    try
    {
      final BluetoothSocket socket = connectSocket(address, connection);
      post(connection, NoGps::nativeOnElm327Connected);
      final InputStream in = socket.getInputStream();
      final byte[] buffer = new byte[256];
      while (true)
      {
        final int read = in.read(buffer);
        if (read < 0)
          throw new IOException("Connection closed");
        final byte[] bytes = Arrays.copyOf(buffer, read);
        post(connection, () -> NoGps.nativeOnElm327Bytes(bytes));
      }
    }
    catch (IOException | SecurityException e)
    {
      final String reason = e.getMessage() != null ? e.getMessage() : e.getClass().getSimpleName();
      post(connection, () -> NoGps.nativeOnElm327Closed(reason));
    }
  }

  @SuppressLint("MissingPermission") // BLUETOOTH_CONNECT is checked when the adapter is chosen.
  @NonNull
  private BluetoothSocket connectSocket(@NonNull String address, int connection) throws IOException
  {
    final BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
    if (adapter == null || !adapter.isEnabled())
      throw new IOException("Bluetooth is off");
    final BluetoothDevice device = adapter.getRemoteDevice(address);
    BluetoothSocket socket = device.createRfcommSocketToServiceRecord(SPP_UUID);
    mSocket = socket;
    try
    {
      socket.connect();
    }
    catch (IOException e)
    {
      // Many cheap clones work only with an insecure connection. A closed connection is not tried again: it would
      // block the adapter for a new one for seconds.
      closeQuietly(socket);
      if (connection != mConnection)
        throw e;
      socket = device.createInsecureRfcommSocketToServiceRecord(SPP_UUID);
      mSocket = socket;
      socket.connect();
    }
    // Closed while connecting: the adapter must not be kept busy.
    if (connection != mConnection)
    {
      closeQuietly(socket);
      throw new IOException("Closed");
    }
    Logger.i(TAG, "Connected to " + address);
    return socket;
  }

  private void post(int connection, @NonNull Runnable task)
  {
    mMainHandler.post(() -> {
      if (connection == mConnection)
        task.run();
    });
  }

  private void closeSocket()
  {
    final BluetoothSocket socket = mSocket;
    mSocket = null;
    if (socket != null)
      closeQuietly(socket);
  }

  private static void closeQuietly(@NonNull BluetoothSocket socket)
  {
    try
    {
      socket.close();
    }
    catch (IOException e)
    {
      Logger.w(TAG, "Failed to close the socket: " + e.getMessage());
    }
  }
}
