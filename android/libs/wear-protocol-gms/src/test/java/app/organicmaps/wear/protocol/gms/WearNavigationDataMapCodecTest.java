package app.organicmaps.wear.protocol.gms;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import app.organicmaps.wear.protocol.WearNavigationData;
import app.organicmaps.wear.protocol.WearNavigationMode;
import app.organicmaps.wear.protocol.WearNavigationState;
import com.google.android.gms.wearable.DataMap;
import org.junit.Test;

public class WearNavigationDataMapCodecTest
{
  @Test
  public void roundTripNavigation()
  {
    final DataMap dataMap = new DataMap();
    WearNavigationDataMapCodec.encode(dataMap, WearNavigationState.navigation());

    assertEquals(WearNavigationMode.NAVIGATION, WearNavigationDataMapCodec.decode(dataMap).getMode());
  }

  @Test
  public void roundTripNormal()
  {
    final DataMap dataMap = new DataMap();
    WearNavigationDataMapCodec.encode(dataMap, WearNavigationState.normal());

    assertEquals(WearNavigationMode.NORMAL, WearNavigationDataMapCodec.decode(dataMap).getMode());
  }

  @Test
  public void encodeUsesStableSchema()
  {
    final DataMap normalDataMap = new DataMap();
    WearNavigationDataMapCodec.encode(normalDataMap, WearNavigationState.normal());

    assertEquals(WearNavigationData.VERSION, normalDataMap.getInt("version", -1));
    assertEquals("NORMAL", normalDataMap.getString("mode"));

    final DataMap navigationDataMap = new DataMap();
    WearNavigationDataMapCodec.encode(navigationDataMap, WearNavigationState.navigation());

    assertEquals(WearNavigationData.VERSION, navigationDataMap.getInt("version", -1));
    assertEquals("NAVIGATION", navigationDataMap.getString("mode"));
  }

  @Test
  public void versionMismatchReturnsNull()
  {
    assertNull(WearNavigationDataMapCodec.decode(dataMap(999, "NAVIGATION")));
  }

  @Test
  public void missingVersionReturnsNull()
  {
    final DataMap dataMap = new DataMap();
    dataMap.putString(WearNavigationData.KEY_MODE, "NAVIGATION");

    assertNull(WearNavigationDataMapCodec.decode(dataMap));
  }

  @Test
  public void missingModeReturnsNull()
  {
    final DataMap dataMap = new DataMap();
    dataMap.putInt(WearNavigationData.KEY_VERSION, WearNavigationData.VERSION);

    assertNull(WearNavigationDataMapCodec.decode(dataMap));
  }

  @Test
  public void unknownModeReturnsNull()
  {
    assertNull(WearNavigationDataMapCodec.decode(dataMap(WearNavigationData.VERSION, "BOGUS")));
  }

  @Test
  public void unknownFieldIsIgnored()
  {
    final DataMap dataMap = dataMap(WearNavigationData.VERSION, "NAVIGATION");
    dataMap.putBoolean("unknown", true);

    assertEquals(WearNavigationMode.NAVIGATION, WearNavigationDataMapCodec.decode(dataMap).getMode());
  }

  private static DataMap dataMap(int version, String mode)
  {
    final DataMap dataMap = new DataMap();
    dataMap.putInt(WearNavigationData.KEY_VERSION, version);
    dataMap.putString(WearNavigationData.KEY_MODE, mode);
    return dataMap;
  }
}
