package app.organicmaps.sdk.display;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.robolectric.Shadows.shadowOf;

import android.os.Looper;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class DisplayManagerTest
{
  private final DisplayManager mDisplay = new DisplayManager();
  private final DisplayChangedListener mCar = mock(DisplayChangedListener.class);
  private final DisplayChangedListener mDevice = mock(DisplayChangedListener.class);
  private MockedStatic<Logger> mLogger;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
  }

  @After
  public void tearDown()
  {
    mLogger.close();
  }

  @Test
  public void disconnectWithoutAPhoneListenerRestoresDeviceSelectionAndAllowsReconnection()
  {
    mDisplay.init(DisplayType.Car);
    mDisplay.addListener(DisplayType.Car, mCar);

    mDisplay.removeListener(DisplayType.Car);
    shadowOf(Looper.getMainLooper()).idle();
    mDisplay.init(DisplayType.Device);

    assertFalse(mDisplay.isCarConnected());
    assertTrue(mDisplay.isDeviceDisplayUsed());
    verifyNoInteractions(mCar);

    mDisplay.addListener(DisplayType.Car, mCar);
    assertTrue(mDisplay.isCarConnected());
    assertTrue(mDisplay.isCarDisplayUsed());
  }

  @Test
  public void disconnectHandsTheDisplayBackToTheConnectedPhone()
  {
    mDisplay.init(DisplayType.Car);
    mDisplay.addListener(DisplayType.Car, mCar);
    mDisplay.addListener(DisplayType.Device, mDevice);

    mDisplay.removeListener(DisplayType.Car);
    shadowOf(Looper.getMainLooper()).idle();

    assertFalse(mDisplay.isCarConnected());
    assertTrue(mDisplay.isDeviceDisplayUsed());
    verify(mDevice).onDisplayChangedToDevice(any());
    verifyNoInteractions(mCar);
  }

  @Test
  public void disconnectDoesNotRestartTheAlreadySelectedPhoneDisplay()
  {
    mDisplay.init(DisplayType.Device);
    mDisplay.addListener(DisplayType.Device, mDevice);
    mDisplay.addListener(DisplayType.Car, mCar);

    mDisplay.removeListener(DisplayType.Car);
    shadowOf(Looper.getMainLooper()).idle();

    assertFalse(mDisplay.isCarConnected());
    assertTrue(mDisplay.isDeviceDisplayUsed());
    verifyNoInteractions(mCar, mDevice);
  }
}
