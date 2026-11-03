package app.organicmaps.widget.placepage;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.location.Location;
import android.widget.TextView;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.bookmarks.data.DistanceAndAzimut;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.widget.ArrowView;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;

public class DirectionFragmentTest
{
  private final DirectionFragment mFragment = mock(DirectionFragment.class, CALLS_REAL_METHODS);
  private final Location mLocation = mock(Location.class);
  private final TextView mBearing = mock(TextView.class);
  private final TextView mDistance = mock(TextView.class);
  private final ArrowView mArrow = mock(ArrowView.class);
  private final Distance mFormattedDistance = mock(Distance.class);

  @Before
  public void setUp() throws ReflectiveOperationException
  {
    final Context context = mock(Context.class);
    doReturn(context).when(mFragment).requireContext();
    final MwmApplication app = mock(MwmApplication.class);
    when(context.getApplicationContext()).thenReturn(app);
    final LocationHelper locations = mock(LocationHelper.class);
    when(app.getLocationHelper()).thenReturn(locations);
    when(locations.getSavedLocation()).thenReturn(mLocation);
    when(mFormattedDistance.toString(context)).thenReturn("100 m");
    setField("mMapObject", mock(MapObject.class));
    setField("mTvAzimuth", mBearing);
    setField("mTvDistance", mDistance);
    setField("mAvDirection", mArrow);
  }

  @Test
  public void wholeDegreeBearingWrapsAtNorth()
  {
    final double[] bearings = {359.4, 359.5, 359.6, 0, 0.4, 0.5, 0.6};
    final String[] expected = {"359°", "0°", "0°", "0°", "0°", "1°", "1°"};
    for (int i = 0; i < bearings.length; i++)
    {
      doReturn(direction(bearings[i])).when(mFragment).getDistanceAndAzimuth(mLocation, 0.0);
      mFragment.onLocationUpdated(mLocation);
      verify(mBearing).setText(expected[i]);
      verify(mDistance).setText("100 m");
      clearInvocations(mBearing, mDistance);
    }
  }

  @Test
  public void turningTheDeviceChangesTheArrowAndPreservesTheAbsoluteTargetBearing()
  {
    doReturn(direction(90)).when(mFragment).getDistanceAndAzimuth(mLocation, 0.0);
    mFragment.onLocationUpdated(mLocation);
    verify(mBearing).setText("90°");
    clearInvocations(mBearing, mDistance);

    final double heading = Math.toRadians(20);
    doReturn(direction(70)).when(mFragment).getDistanceAndAzimuth(mLocation, heading);
    mFragment.onCompassUpdated(heading);
    verify(mArrow).setAzimuth(Math.toRadians(70));
    final double turnedHeading = Math.toRadians(110);
    doReturn(direction(340)).when(mFragment).getDistanceAndAzimuth(mLocation, turnedHeading);
    mFragment.onCompassUpdated(turnedHeading);
    verify(mArrow).setAzimuth(Math.toRadians(340));
    verifyNoInteractions(mBearing, mDistance);

    mFragment.onLocationUpdated(mLocation);
    verify(mBearing).setText("90°");
    verify(mFragment, times(2)).getDistanceAndAzimuth(mLocation, 0.0);
  }

  private DistanceAndAzimut direction(double degrees)
  {
    return new DistanceAndAzimut(mFormattedDistance, Math.toRadians(degrees));
  }

  private void setField(String name, Object value) throws ReflectiveOperationException
  {
    final Field field = DirectionFragment.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(mFragment, value);
  }
}
