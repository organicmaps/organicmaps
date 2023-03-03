package app.organicmaps.display;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleEventObserver;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.MwmApplication;
import app.organicmaps.util.log.Logger;

public class DisplayManager implements Handler.Callback
{
  private static final String TAG = DisplayManager.class.getSimpleName();

  // See description in updateDisplay()
  private static final int MESSAGE_CHANGE_DISPLAY_TO_CAR = 0x01;
  private static final int MESSAGE_CHANGE_DISPLAY_TO_DEVICE = 0x02;
  private static final int MESSAGE_DELAY = 100; // 0.1s

  private class DisplayLifecycleObserver implements LifecycleEventObserver
  {
    @NonNull
    private final DisplayHolder displayHolder;

    public DisplayLifecycleObserver(@NonNull DisplayHolder displayHolder)
    {
      this.displayHolder = displayHolder;
    }

    @Override
    public void onStateChanged(@NonNull LifecycleOwner source, @NonNull Lifecycle.Event event)
    {
      Logger.d(TAG, "Source: " + source + " Event: " + event);
      displayHolder.lastEvent = event;

      updateDisplay();
    }
  }

  private static class DisplayHolder
  {
    boolean isConnected = false;
    boolean notify = false;
    Lifecycle.Event lastEvent;
    DisplayChangedListener listener;
    DisplayLifecycleObserver lifecycleObserver;

    public void destroy()
    {
      isConnected = false;
      notify = false;
      lastEvent = null;
      listener = null;
      lifecycleObserver = null;
    }
  }

  @NonNull
  private DisplayType mCurrentDisplayType = DisplayType.Device;
  @Nullable
  private DisplayHolder mDevice;
  @Nullable
  private DisplayHolder mCar;

  private final Handler mCallbackHandler = new Handler(Looper.getMainLooper(), this);

  @NonNull
  public static DisplayManager from(@NonNull Context context)
  {
    final MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getDisplayManager();
  }

  public boolean isCarConnected()
  {
    return mCar != null;
  }

  public boolean isDeviceConnected()
  {
    return mDevice != null;
  }

  public boolean isCarDisplayUsed()
  {
    return mCurrentDisplayType == DisplayType.Car;
  }

  public boolean isDeviceDisplayUsed()
  {
    return mCurrentDisplayType == DisplayType.Device;
  }

  public void addListener(@NonNull final DisplayType displayType, @NonNull final DisplayChangedListener listener)
  {
    Logger.d(TAG, "displayType = " + displayType + ", listener = " + listener);

    if (displayType == DisplayType.Device && mDevice == null)
    {
      mDevice = new DisplayHolder();
      mDevice.isConnected = true;
      mDevice.notify = true;
      mDevice.listener = listener;
    }
    else if (displayType == DisplayType.Car && mCar == null)
    {
      mCar = new DisplayHolder();
      mCar.isConnected = true;
      mCar.notify = true;
      mCar.listener = listener;
    }

    if (isCarConnected() && !isDeviceConnected())
      mCurrentDisplayType = displayType;
  }

  public LifecycleEventObserver getObserverFor(@NonNull final DisplayType displayType)
  {
    Logger.d(TAG, "displayType = " + displayType);

    if (displayType == DisplayType.Device && mDevice != null)
      return mDevice.lifecycleObserver = new DisplayLifecycleObserver(mDevice);
    else if (displayType == DisplayType.Car && mCar != null)
      return mCar.lifecycleObserver = new DisplayLifecycleObserver(mCar);

    throw new RuntimeException("Display holder for " + displayType + " is not initialized");
  }

  private void updateDisplay()
  {
    Logger.d(TAG);

    final boolean isDeviceDestroyed = mDevice != null && mDevice.lastEvent == Lifecycle.Event.ON_DESTROY;
    final boolean isCarDestroyed = mCar != null && mCar.lastEvent == Lifecycle.Event.ON_DESTROY;

    // Device is destroyed due to the one of the following reasons:
    // 1. Configuration changes (rotation, theme and so on)
    // 2. Entering navigation mode
    // 3. Application is closed
    // When Car is not connected we don't need to change display type.
    // If device was destroyed due to reasons 1-2,
    // mDevice will be created with the actual information about the new activity after Activity.onCreate() call
    if (isDeviceDestroyed)
    {
      destroyDisplayHolder(mDevice);

      // Process configuration change case (also works for navigation case)
      if (isDeviceDisplayUsed() && isCarConnected())
        // We can't check whether the device changing its configuration or not
        // Let's post a delayed message to the handler notifying that display should be changed to car if device is still not connected
        mCallbackHandler.sendEmptyMessageDelayed(MESSAGE_CHANGE_DISPLAY_TO_CAR, MESSAGE_DELAY);
      return;
    }

    // This happens when Android Auto is disconnected or when the application is stopped (if only car connected)
    if (isCarDestroyed)
    {
      destroyDisplayHolder(mCar);

      if (isDeviceConnected())
      {
        mDevice.notify = true;
        notifyDisplayChange(DisplayType.Device);
      }

      // We don't care about what's going on next when car and device are disconnected -> application will be stopped
      return;
    }

    // Only device connected. The most common case
    if (isDeviceConnected() && !isCarConnected())
    {
      if (mCurrentDisplayType != DisplayType.Device)
      {
        mDevice.notify = true;
        notifyDisplayChange(DisplayType.Device);
        return;
      }
    }

    // Only car connected
    if (isCarConnected() && !isDeviceConnected())
    {
      if (mCurrentDisplayType != DisplayType.Car)
      {
        mCar.notify = true;
        notifyDisplayChange(DisplayType.Car);
        return;
      }
    }

    if (isDeviceConnected() && isCarConnected())
    {
      // 1. Map was used on Car
      // 2. Car disconnected
      // 3. Show map on Device
      // Activity is being recreated when Android Auto disconnected. We have to handle it
      if (isCarDisplayUsed() && mDevice.lastEvent == Lifecycle.Event.ON_START)
      {
        // Let's post a delayed message to the handler notifying that display should be changed to device if car is disconnected
        mCallbackHandler.sendEmptyMessageDelayed(MESSAGE_CHANGE_DISPLAY_TO_DEVICE, MESSAGE_DELAY);
        return;
      }

      // 1. Map was shown on Device
      // 2. MwmActivity.onStop() called
      // 3. Car is launched
      // Let's show map on Car because it's the case when we hide application on the phone without closing it
      if (isDeviceDisplayUsed() && mDevice.lastEvent == Lifecycle.Event.ON_STOP && mCar.lastEvent == Lifecycle.Event.ON_CREATE)
      {
        mDevice.notify = true;
        mCar.notify = true;
        notifyDisplayChange(DisplayType.Car);
        return;
      }

      // Car was connected after Device
      // Notify car
      if (mCar.lastEvent == Lifecycle.Event.ON_CREATE)
      {
        mCar.notify = true;
        notifyDisplayChange(mCurrentDisplayType);
        return;
      }

      // Device was connected after Car
      // Notify device
      if (mDevice.lastEvent == Lifecycle.Event.ON_CREATE)
      {
        mDevice.notify = true;
        notifyDisplayChange(mCurrentDisplayType);
        return;
      }

      // 1. Map was shown on Device
      // 2. MwmActivity.onStop() called
      // 3. Map is shown on Car
      // Reason: we need to disable controls on device
      if (mDevice.lastEvent == Lifecycle.Event.ON_RESUME && isCarDisplayUsed())
      {
        mDevice.notify = true;
        notifyDisplayChange(mCurrentDisplayType);
      }
    }
  }

  private void notifyDisplayChange(@NonNull final DisplayType newDisplayType)
  {
    Logger.d(TAG, "newDisplayType = " + newDisplayType);

    mCurrentDisplayType = newDisplayType;

    if (mCurrentDisplayType == DisplayType.Device)
      mCallbackHandler.post(this::onDisplayTypeChangedToDevice);
    else if (mCurrentDisplayType == DisplayType.Car)
      mCallbackHandler.post(this::onDisplayTypeChangedToCar);
  }

  public void changeDisplay(@NonNull final DisplayType newDisplayType)
  {
    Logger.d(TAG, "newDisplayType = " + newDisplayType);

    if (mCurrentDisplayType == newDisplayType)
      return;

    if (isCarConnected())
    {
      assert mCar != null;
      mCar.notify = true;
    }
    if (isDeviceConnected())
    {
      assert mDevice != null;
      mDevice.notify = true;
    }

    notifyDisplayChange(newDisplayType);
  }

  private void onDisplayTypeChangedToDevice()
  {
    Logger.d(TAG);

    if (mCar != null && mCar.notify)
    {
      mCar.listener.onDisplayChanged(DisplayType.Device);
      mCar.notify = false;
    }
    if (mDevice != null && mDevice.notify)
    {
      mDevice.listener.onDisplayChanged(DisplayType.Device);
      mDevice.notify = false;
    }
  }

  private void onDisplayTypeChangedToCar()
  {
    Logger.d(TAG);

    if (mDevice != null && mDevice.notify)
    {
      mDevice.listener.onDisplayChanged(DisplayType.Car);
      mDevice.notify = false;
    }
    if (mCar != null && mCar.notify)
    {
      mCar.listener.onDisplayChanged(DisplayType.Car);
      mCar.notify = false;
    }
  }

  private void destroyDisplayHolder(DisplayHolder holder)
  {
    if (holder == null)
      return;
    Logger.d(TAG, holder == mDevice ? "device" : "car");

    holder.destroy();
    if (holder == mDevice)
      mDevice = null;
    else if (holder == mCar)
      mCar = null;
  }

  @Override
  public boolean handleMessage(@NonNull Message msg)
  {
    Logger.d(TAG, "" + msg.what);
    if (msg.what == MESSAGE_CHANGE_DISPLAY_TO_CAR)
    {
      // Device still not connected. It's not a configuration change case. Switch display to car
      if (!isDeviceConnected() && isCarConnected())
      {
        assert mCar != null;
        mCar.notify = true;
        notifyDisplayChange(DisplayType.Car);
      }
      return true;
    }
    else if (msg.what == MESSAGE_CHANGE_DISPLAY_TO_DEVICE)
    {
      // Car disconnected. Let's switch display to Device
      if (isDeviceConnected() && !isCarConnected())
      {
        assert mDevice != null;
        mDevice.notify = true;
        notifyDisplayChange(DisplayType.Device);
      }
      return true;
    }
    return false;
  }
}
