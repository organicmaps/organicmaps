package app.organicmaps.car;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.RETURNS_SELF;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.withSettings;

import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.DurationSpan;
import androidx.car.app.model.Row;
import androidx.car.app.navigation.model.Maneuver;
import androidx.car.app.navigation.model.TravelEstimate;
import androidx.car.app.navigation.model.Trip;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.sdk.car.RoutingHelpers;
import app.organicmaps.sdk.car.RoutingUtils;
import app.organicmaps.sdk.routing.CarDirection;
import app.organicmaps.sdk.routing.PedestrianDirection;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.widget.roadshield.RoadShieldUtils;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.time.Instant;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class RoutingEstimatesTest
{
  private static final int[] CASES = {
      0, 20, 59, 89, 90, 269, 270, 1649, 1650, 1665, 2085, 3569, 3570, 3630, 86400, 94485, Integer.MAX_VALUE};

  @Test
  public void destinationAndArrivalPreserveRawSeconds() throws Exception
  {
    CarContext context = mock(CarContext.class);
    try (MockedStatic<RoutingHelpers> helpers = mockStatic(RoutingHelpers.class, CALLS_REAL_METHODS);
         MockedStatic<RoadShieldUtils> shields = mockStatic(RoadShieldUtils.class);
         MockedStatic<TextUtils> text = mockStatic(TextUtils.class))
    {
      helpers.when(() -> RoutingHelpers.createManeuver(any(), any(), anyInt()))
          .thenReturn(new Maneuver.Builder(Maneuver.TYPE_STRAIGHT).build());
      shields
          .when(() -> RoadShieldUtils.createStreetTextWithShields(any(), anyString(), any(), anyFloat(), anyBoolean()))
          .thenAnswer(invocation -> invocation.getArgument(1));
      text.when(() -> TextUtils.isEmpty(any())).thenAnswer(invocation -> {
        CharSequence value = invocation.getArgument(0);
        return value == null || value.length() == 0;
      });

      for (int seconds : CASES)
      {
        RoutingInfo info = createRoutingInfo(seconds);
        long before = Instant.now().plusSeconds(seconds).getEpochSecond();
        Trip trip = RoutingUtils.createTrip(context, info, null, CarColor.DEFAULT);
        long after = Instant.now().plusSeconds(seconds).getEpochSecond();

        TravelEstimate destination = trip.getDestinationTravelEstimates().get(0);
        assertEquals(seconds + " seconds", seconds, destination.getRemainingTimeSeconds());
        long arrival = destination.getArrivalTimeAtDestination().getTimeSinceEpochMillis() / 1000;
        assertTrue("Arrival should use the raw duration", arrival >= before && arrival <= after);
        assertEquals(seconds, info.totalTimeInSeconds);
        assertEquals(2, trip.getStepTravelEstimates().size());
        for (TravelEstimate step : trip.getStepTravelEstimates())
          assertEquals(0, step.getRemainingTimeSeconds());
      }
    }
  }

  @Test
  public void routePreviewPreservesRawSecondsForTheHost() throws Exception
  {
    PlaceScreen screen = mock(PlaceScreen.class, CALLS_REAL_METHODS);
    Method getRouteInfo = PlaceScreen.class.getDeclaredMethod("getPlaceRouteInfo", RoutingInfo.class);
    getRouteInfo.setAccessible(true);

    for (int seconds : CASES)
    {
      RoutingInfo info = createRoutingInfo(seconds);
      try (MockedConstruction<SpannableString> spans = mockConstruction(SpannableString.class);
           MockedConstruction<Row.Builder> rows =
               mockConstruction(Row.Builder.class, withSettings().defaultAnswer(RETURNS_SELF)))
      {
        getRouteInfo.invoke(screen, info);

        ArgumentCaptor<DurationSpan> duration = ArgumentCaptor.forClass(DurationSpan.class);
        verify(spans.constructed().get(0))
            .setSpan(duration.capture(), eq(0), eq(1), eq(Spanned.SPAN_INCLUSIVE_INCLUSIVE));
        assertEquals(seconds + " seconds", seconds, duration.getValue().getDurationSeconds());
        assertEquals(seconds, info.totalTimeInSeconds);
      }
    }
  }

  private static RoutingInfo createRoutingInfo(int seconds) throws Exception
  {
    // RoutingInfo is normally constructed by JNI.
    Constructor<?> constructor = RoutingInfo.class.getDeclaredConstructors()[0];
    constructor.setAccessible(true);
    return (RoutingInfo) constructor.newInstance(new Distance(100, "100", (byte) 0), new Distance(10, "10", (byte) 0),
                                                 "Current street", "Next street", null, "Following street", null, 0.0,
                                                 CarDirection.GoStraight, CarDirection.TurnRight,
                                                 PedestrianDirection.GoStraight, 0, seconds, null, -1.0, false, false);
  }
}
