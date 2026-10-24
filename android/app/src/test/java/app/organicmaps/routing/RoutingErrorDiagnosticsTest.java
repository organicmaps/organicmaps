package app.organicmaps.routing;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.ResultCodes;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import java.util.Locale;
import org.junit.Test;

public class RoutingErrorDiagnosticsTest
{
  @Test
  public void reportKeepsAllCoordinatesReadableInCommaDecimalLocales()
  {
    Locale previous = Locale.getDefault();
    try
    {
      Locale.setDefault(Locale.forLanguageTag("ru-RU"));
      RouteMarkData[] points = {point(RouteMarkType.Start, 0, true, 52.375208, 5.243226),
                                point(RouteMarkType.Intermediate, 0, false, 52.35, 5.24),
                                point(RouteMarkType.Finish, 0, false, 52.323037, 5.229683)};
      String report = RoutingErrorDiagnostics.format("2026.10.24-test", Router.Water, ResultCodes.ROUTE_NOT_FOUND,
                                                     points, new String[] {"Flevoland", "Flevoland", "Amsterdam"});
      assertTrue(report.contains("App: 2026.10.24-test\nMode: Water\nResult code: 8"));
      assertTrue(report.contains("Start: 52.37520800, 5.24322600 (My position)\n  Map: Flevoland"));
      assertTrue(report.contains("Stop 1: 52.35000000, 5.24000000"));
      assertTrue(report.contains("Finish: 52.32303700, 5.22968300\n  Map: Amsterdam"));
    }
    finally
    {
      Locale.setDefault(previous);
    }
  }

  @Test
  public void errorWithoutRoutePointsStillIdentifiesBuildModeAndResult()
  {
    String report = RoutingErrorDiagnostics.format("debug-build", Router.Pedestrian, ResultCodes.NO_POSITION,
                                                   new RouteMarkData[0], new String[0]);
    assertTrue(report.contains("App: debug-build\nMode: Pedestrian\nResult code: 2"));
    assertFalse(report.contains("Start:"));
  }

  private static RouteMarkData point(RouteMarkType type, int index, boolean myPosition, double lat, double lon)
  {
    return new RouteMarkData(null, null, type, index, true, myPosition, false, lat, lon);
  }
}
