package app.organicmaps.sdk.location;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothStatusCodes;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanRecord;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayDeque;
import java.util.Collections;
import java.util.UUID;

/**
 * The Bluetooth LE connection to the NoGPS ESP32 sensor box for the navigation without GPS in the core: the core
 * talks to the box, this class searches the box by its service, connects, subscribes to its lines and passes the
 * received bytes back on the UI thread. It keeps doing it until closed: the box switches its radio off while the
 * car is parked. The service and the pairing are described in docs/nogps/esp32-firmware-spec.md, section 17.
 */
class Esp32BleTransport
{
  // nogps::BleState.
  static final int STATE_OFF = 0;
  static final int STATE_NO_PERMISSION = 1;
  static final int STATE_DISABLED = 2;
  static final int STATE_SEARCHING = 3;
  static final int STATE_PAIRING_CLOSED = 4;
  static final int STATE_CONNECTING = 5;
  static final int STATE_PAIRING = 6;
  static final int STATE_CONNECTED = 7;

  private static final String TAG = Esp32BleTransport.class.getSimpleName();
  private static final UUID SERVICE = UUID.fromString("02CB0001-C0C3-40D0-819C-5A85998DCD99");
  // The lines of the box, notified.
  private static final UUID TX = UUID.fromString("02CB0002-C0C3-40D0-819C-5A85998DCD99");
  // The commands to the box.
  private static final UUID RX = UUID.fromString("02CB0003-C0C3-40D0-819C-5A85998DCD99");
  // The pieces of a new firmware of the box, section 18 of the same document.
  private static final UUID OTA = UUID.fromString("02CB0004-C0C3-40D0-819C-5A85998DCD99");
  private static final UUID CLIENT_CONFIG = UUID.fromString("00002902-0000-1000-8000-00805F9B34FB");
  private static final String NAME_PREFIX = "NoGPS-";
  // The box tells in its advertisement if it accepts a new phone now: company, format version 1, flags.
  private static final int MANUFACTURER_ID = 0xFFFF;
  private static final int FLAG_PAIRING_OPEN = 1;
  // A line of the box fits a notification with it, the default of 23 has no room even for a command.
  private static final int MTU = 247;
  private static final long RETRY_DELAY_MS = 2000;
  // A box that is found but drops the connection again and again is tried less and less often.
  private static final long MAX_RETRY_DELAY_MS = 60_000;
  // Android turns a scan lasting half an hour into one that finds nothing, so it is started anew.
  private static final long RESCAN_INTERVAL_MS = 5 * 60 * 1000;
  // A new box waits this long for a paired one to be found: both may be near.
  private static final long NEW_BOX_DELAY_MS = 3000;
  // The box doesn't answer the commands, a few of them wait for the previous one to leave at most.
  private static final int MAX_OUTGOING = 16;
  // The box takes the pieces of a firmware 4096 bytes ahead of what it has written: 17 pieces.
  private static final int MAX_FIRMWARE_PIECES = 64;
  private static final long WRITE_RETRY_MS = 20;

  @NonNull
  private final Context mContext;
  private final Handler mMainHandler = new Handler(Looper.getMainLooper());
  private boolean mOpen;
  private int mState = STATE_OFF;
  @Nullable
  private BluetoothLeScanner mScanner;
  @Nullable
  private ScanCallback mScanCallback;
  private long mScanStartMs;
  @Nullable
  private BluetoothGatt mGatt;
  @Nullable
  private BluetoothGattCharacteristic mRx;
  // A box with an old firmware has none.
  @Nullable
  private BluetoothGattCharacteristic mOta;
  // Subscribed to the lines of the box.
  private boolean mReady;
  private int mMtu = 23;
  private final ArrayDeque<byte[]> mOutgoing = new ArrayDeque<>();
  // The pieces of a firmware go after the lines: a line a second keeps the connection with the box.
  private final ArrayDeque<byte[]> mFirmware = new ArrayDeque<>();
  private boolean mWriting;
  private final Runnable mWriter = this::writeNext;
  private long mRetryDelayMs = RETRY_DELAY_MS;
  private final Runnable mStarter = this::start;

  private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent)
    {
      if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(intent.getAction()))
        onAdapterStateChanged(intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR));
      else if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(intent.getAction()))
        onBondStateChanged(intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE),
                           intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR));
    }
  };

  Esp32BleTransport(@NonNull Context context)
  {
    mContext = context;
  }

  @UiThread
  void open()
  {
    close();
    mOpen = true;
    mRetryDelayMs = RETRY_DELAY_MS;
    final IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
    filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
    ContextCompat.registerReceiver(mContext, mReceiver, filter, ContextCompat.RECEIVER_NOT_EXPORTED);
    start();
  }

  @UiThread
  void send(@NonNull byte[] line)
  {
    if (!mReady)
      return;
    if (line.length > mMtu - 3)
    {
      Logger.w(TAG, "A line of " + line.length + " bytes doesn't fit MTU " + mMtu);
      return;
    }
    if (mOutgoing.size() >= MAX_OUTGOING)
      mOutgoing.poll();
    mOutgoing.add(line);
    writeNext();
  }

  /**
   * A piece of a new firmware of the box. A piece that doesn't fit is dropped: the box tells where it has
   * stopped, and the core sends from there again.
   */
  @UiThread
  void sendFirmware(@NonNull byte[] piece)
  {
    if (!mReady || mOta == null || piece.length > mMtu - 3 || mFirmware.size() >= MAX_FIRMWARE_PIECES)
      return;
    mFirmware.add(piece);
    writeNext();
  }

  @UiThread
  void close()
  {
    if (!mOpen)
      return;
    mOpen = false;
    mMainHandler.removeCallbacks(mStarter);
    mContext.unregisterReceiver(mReceiver);
    stopScan();
    closeGatt();
    mState = STATE_OFF;
  }

  private boolean hasPermission()
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
      return isGranted(Manifest.permission.BLUETOOTH_SCAN) && isGranted(Manifest.permission.BLUETOOTH_CONNECT);
    // A scan needs the location permission before Android 12.
    return isGranted(Manifest.permission.ACCESS_FINE_LOCATION);
  }

  private boolean isGranted(@NonNull String permission)
  {
    return ContextCompat.checkSelfPermission(mContext, permission) == PackageManager.PERMISSION_GRANTED;
  }

  private void setState(int state)
  {
    if (state == mState)
      return;
    Logger.i(TAG, "state = " + state);
    mState = state;
    NoGps.nativeOnEsp32BleState(state);
  }

  /**
   * Searches the box, or waits until it can be searched.
   */
  @SuppressLint("MissingPermission") // Checked by hasPermission().
  private void start()
  {
    mMainHandler.removeCallbacks(mStarter);
    if (!mOpen || mGatt != null)
      return;
    stopScan();
    final BluetoothManager manager = (BluetoothManager) mContext.getSystemService(Context.BLUETOOTH_SERVICE);
    final BluetoothAdapter adapter = manager != null ? manager.getAdapter() : null;
    if (adapter == null || !mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE))
    {
      setState(STATE_OFF);
      return;
    }
    if (!hasPermission())
    {
      // The user is asked for it by the sensors screen, nobody tells when it is given.
      setState(STATE_NO_PERMISSION);
      mMainHandler.postDelayed(mStarter, RETRY_DELAY_MS);
      return;
    }
    final BluetoothLeScanner scanner = adapter.isEnabled() ? adapter.getBluetoothLeScanner() : null;
    if (scanner == null)
    {
      // ACTION_STATE_CHANGED tells when it is switched on.
      setState(STATE_DISABLED);
      return;
    }

    final ScanCallback callback = new ScanCallback() {
      @Override
      public void onScanResult(int callbackType, ScanResult result)
      {
        if (this == mScanCallback)
          onBoxFound(adapter, result);
      }

      @Override
      public void onScanFailed(int errorCode)
      {
        Logger.w(TAG, "Scan failed: " + errorCode);
        if (this != mScanCallback)
          return;
        mScanCallback = null;
        mMainHandler.postDelayed(mStarter, RETRY_DELAY_MS);
      }
    };
    final ScanFilter filter = new ScanFilter.Builder().setServiceUuid(new ParcelUuid(SERVICE)).build();
    final ScanSettings settings = new ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_BALANCED).build();
    try
    {
      scanner.startScan(Collections.singletonList(filter), settings, callback);
    }
    catch (SecurityException | IllegalStateException e)
    {
      Logger.w(TAG, "Can't scan: " + e.getMessage());
      mMainHandler.postDelayed(mStarter, RETRY_DELAY_MS);
      return;
    }
    mScanner = scanner;
    mScanCallback = callback;
    mScanStartMs = SystemClock.elapsedRealtime();
    setState(STATE_SEARCHING);
    mMainHandler.postDelayed(mStarter, RESCAN_INTERVAL_MS);
  }

  @SuppressLint("MissingPermission")
  private void stopScan()
  {
    final BluetoothLeScanner scanner = mScanner;
    final ScanCallback callback = mScanCallback;
    mScanner = null;
    mScanCallback = null;
    if (scanner == null || callback == null)
      return;
    try
    {
      scanner.stopScan(callback);
    }
    catch (SecurityException | IllegalStateException e)
    {
      // Bluetooth has been switched off or the permission is taken back: the scan is over anyway.
      Logger.w(TAG, "Can't stop the scan: " + e.getMessage());
    }
  }

  @SuppressLint("MissingPermission")
  private void onBoxFound(@NonNull BluetoothAdapter adapter, @NonNull ScanResult result)
  {
    final BluetoothDevice device = result.getDevice();
    final boolean bonded = device.getBondState() == BluetoothDevice.BOND_BONDED;
    if (!bonded)
    {
      // The box accepts a new phone for two minutes after it is powered, the phone asks for its code.
      final ScanRecord record = result.getScanRecord();
      final byte[] data = record != null ? record.getManufacturerSpecificData(MANUFACTURER_ID) : null;
      final boolean pairingOpen = data != null && data.length >= 2 && (data[1] & FLAG_PAIRING_OPEN) != 0;
      if (!pairingOpen)
      {
        setState(STATE_PAIRING_CLOSED);
        return;
      }
      // A box paired already is taken first if it is near: the new one may be somebody else's. Without it the
      // new box is a second or a replaced one of the user.
      if (hasPairedBox(adapter) && SystemClock.elapsedRealtime() - mScanStartMs < NEW_BOX_DELAY_MS)
        return;
    }
    Logger.i(TAG, "Found " + device.getAddress() + " rssi " + result.getRssi() + " bonded " + bonded);
    mMainHandler.removeCallbacks(mStarter);
    stopScan();
    setState(STATE_CONNECTING);
    mMtu = 23;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
      mGatt = device.connectGatt(mContext, false, mGattCallback, BluetoothDevice.TRANSPORT_LE);
    else
      mGatt = device.connectGatt(mContext, false, mGattCallback);
    if (mGatt == null)
      restartLater();
  }

  @SuppressLint("MissingPermission")
  private boolean hasPairedBox(@NonNull BluetoothAdapter adapter)
  {
    for (BluetoothDevice device : adapter.getBondedDevices())
    {
      final String name = device.getName();
      if (name != null && name.startsWith(NAME_PREFIX))
        return true;
    }
    return false;
  }

  @SuppressLint("MissingPermission")
  private void closeGatt()
  {
    final BluetoothGatt gatt = mGatt;
    mGatt = null;
    mRx = null;
    mOta = null;
    mReady = false;
    mWriting = false;
    mOutgoing.clear();
    mFirmware.clear();
    mMainHandler.removeCallbacks(mWriter);
    if (gatt == null)
      return;
    try
    {
      gatt.disconnect();
      gatt.close();
    }
    catch (SecurityException e)
    {
      Logger.w(TAG, "Can't close: " + e.getMessage());
    }
  }

  private void restartLater()
  {
    final boolean worked = mReady;
    closeGatt();
    if (!mOpen)
      return;
    setState(STATE_SEARCHING);
    mRetryDelayMs = worked ? RETRY_DELAY_MS : Math.min(MAX_RETRY_DELAY_MS, mRetryDelayMs * 2);
    mMainHandler.removeCallbacks(mStarter);
    mMainHandler.postDelayed(mStarter, mRetryDelayMs);
  }

  private void onAdapterStateChanged(int state)
  {
    if (state == BluetoothAdapter.STATE_ON)
    {
      start();
    }
    else if (state == BluetoothAdapter.STATE_OFF || state == BluetoothAdapter.STATE_TURNING_OFF)
    {
      mMainHandler.removeCallbacks(mStarter);
      mScanner = null;
      mScanCallback = null;
      closeGatt();
      setState(STATE_DISABLED);
    }
  }

  private void onBondStateChanged(@Nullable BluetoothDevice device, int bondState)
  {
    final BluetoothGatt gatt = mGatt;
    if (gatt == null || device == null || !device.equals(gatt.getDevice()))
      return;
    Logger.i(TAG, "bond = " + bondState);
    if (bondState == BluetoothDevice.BOND_BONDING)
    {
      setState(STATE_PAIRING);
    }
    else if (bondState == BluetoothDevice.BOND_BONDED)
    {
      if (mReady)
        setState(STATE_CONNECTED);
      else if (mRx != null)
        subscribe(gatt);
    }
    else if (bondState == BluetoothDevice.BOND_NONE)
    {
      // A wrong code, the user has refused or the box doesn't accept a new phone any more.
      restartLater();
    }
  }

  private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
    @Override
    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState)
    {
      mMainHandler.post(() -> onConnection(gatt, status, newState));
    }

    @Override
    public void onMtuChanged(BluetoothGatt gatt, int mtu, int status)
    {
      mMainHandler.post(() -> onMtu(gatt, mtu, status));
    }

    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status)
    {
      mMainHandler.post(() -> onServices(gatt, status));
    }

    @Override
    public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status)
    {
      mMainHandler.post(() -> onSubscribed(gatt, status));
    }

    @Override
    public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status)
    {
      mMainHandler.post(() -> {
        if (gatt != mGatt)
          return;
        mWriting = false;
        writeNext();
      });
    }

    // Before Android 13: the value is read here, another notification may replace it later.
    @Override
    @SuppressWarnings("deprecation")
    public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic)
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
        return;
      final byte[] value = characteristic.getValue();
      if (value != null)
        onBytes(gatt, value.clone());
    }

    @Override
    public void onCharacteristicChanged(@NonNull BluetoothGatt gatt,
                                        @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value)
    {
      onBytes(gatt, value);
    }
  };

  private void onBytes(@NonNull BluetoothGatt gatt, @NonNull byte[] value)
  {
    mMainHandler.post(() -> {
      // The bytes of a closed connection are dropped.
      if (gatt == mGatt)
        NoGps.nativeOnEsp32BleBytes(value);
    });
  }

  @SuppressLint("MissingPermission")
  private void onConnection(@NonNull BluetoothGatt gatt, int status, int newState)
  {
    if (gatt != mGatt)
      return;
    Logger.i(TAG, "connection = " + newState + " status " + status);
    if (newState != BluetoothProfile.STATE_CONNECTED || status != BluetoothGatt.GATT_SUCCESS)
    {
      restartLater();
      return;
    }
    // The connection interval is not asked for: the box asks for the one it needs by itself.
    try
    {
      if (!gatt.requestMtu(MTU))
        restartLater();
    }
    catch (SecurityException e)
    {
      restartLater();
    }
  }

  @SuppressLint("MissingPermission")
  private void onMtu(@NonNull BluetoothGatt gatt, int mtu, int status)
  {
    if (gatt != mGatt)
      return;
    Logger.i(TAG, "mtu = " + mtu + " status " + status);
    if (status == BluetoothGatt.GATT_SUCCESS)
      mMtu = mtu;
    // The box may ask for it too: the services are searched once.
    if (mRx != null)
      return;
    try
    {
      if (!gatt.discoverServices())
        restartLater();
    }
    catch (SecurityException e)
    {
      restartLater();
    }
  }

  private void onServices(@NonNull BluetoothGatt gatt, int status)
  {
    if (gatt != mGatt)
      return;
    final BluetoothGattService service = status == BluetoothGatt.GATT_SUCCESS ? gatt.getService(SERVICE) : null;
    final BluetoothGattCharacteristic rx = service != null ? service.getCharacteristic(RX) : null;
    if (rx == null || service.getCharacteristic(TX) == null)
    {
      Logger.w(TAG, "No service of the box, status " + status);
      restartLater();
      return;
    }
    mRx = rx;
    mOta = service.getCharacteristic(OTA);
    subscribe(gatt);
  }

  @SuppressLint("MissingPermission")
  @SuppressWarnings("deprecation")
  private void subscribe(@NonNull BluetoothGatt gatt)
  {
    final BluetoothGattService service = gatt.getService(SERVICE);
    final BluetoothGattCharacteristic tx = service != null ? service.getCharacteristic(TX) : null;
    final BluetoothGattDescriptor config = tx != null ? tx.getDescriptor(CLIENT_CONFIG) : null;
    if (config == null)
    {
      Logger.w(TAG, "The lines of the box can't be subscribed to");
      restartLater();
      return;
    }
    try
    {
      gatt.setCharacteristicNotification(tx, true);
      final boolean started;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
      {
        started = gatt.writeDescriptor(config, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
               == BluetoothStatusCodes.SUCCESS;
      }
      else
      {
        config.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        started = gatt.writeDescriptor(config);
      }
      if (!started)
        restartLater();
    }
    catch (SecurityException e)
    {
      restartLater();
    }
  }

  private void onSubscribed(@NonNull BluetoothGatt gatt, int status)
  {
    if (gatt != mGatt)
      return;
    Logger.i(TAG, "subscribed, status " + status);
    if (status == BluetoothGatt.GATT_SUCCESS)
    {
      mReady = true;
      setState(STATE_CONNECTED);
    }
    else if (status != BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION
             && status != BluetoothGatt.GATT_INSUFFICIENT_ENCRYPTION)
    {
      restartLater();
    }
    // Otherwise the phone pairs with the box now, see onBondStateChanged().
  }

  @SuppressLint("MissingPermission")
  @SuppressWarnings("deprecation")
  private void writeNext()
  {
    final BluetoothGatt gatt = mGatt;
    final boolean firmware = mOutgoing.isEmpty();
    final ArrayDeque<byte[]> queue = firmware ? mFirmware : mOutgoing;
    final BluetoothGattCharacteristic characteristic = firmware ? mOta : mRx;
    if (mWriting || gatt == null || characteristic == null || queue.isEmpty())
      return;
    final byte[] data = queue.peek();
    try
    {
      final boolean started;
      // Without a response: the box answers a command with a line, and its data are the answer to HELLO. It
      // tells how much of a firmware it has got in lines too.
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
      {
        started = gatt.writeCharacteristic(characteristic, data, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
               == BluetoothStatusCodes.SUCCESS;
      }
      else
      {
        characteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
        characteristic.setValue(data);
        started = gatt.writeCharacteristic(characteristic);
      }
      mWriting = started;
      if (started)
      {
        queue.poll();
      }
      else if (firmware)
      {
        // The pieces go one after another, a refused one is written again in a moment.
        mMainHandler.removeCallbacks(mWriter);
        mMainHandler.postDelayed(mWriter, WRITE_RETRY_MS);
      }
      else
      {
        // A refused write is busy with another operation: the line is dropped, HELLO comes again in a second.
        queue.poll();
        writeNext();
      }
    }
    catch (SecurityException e)
    {
      mOutgoing.clear();
      mFirmware.clear();
    }
  }
}
