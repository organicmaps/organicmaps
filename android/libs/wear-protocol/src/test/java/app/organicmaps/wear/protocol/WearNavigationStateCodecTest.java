package app.organicmaps.wear.protocol;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import java.nio.charset.StandardCharsets;
import org.junit.Test;

public class WearNavigationStateCodecTest
{
  @Test
  public void roundTripNavigation()
  {
    final byte[] bytes = WearNavigationStateCodec.encode(WearNavigationState.navigation());
    final WearNavigationState decoded = WearNavigationStateCodec.decode(bytes);
    assertEquals(WearNavigationMode.NAVIGATION, decoded.getMode());
  }

  @Test
  public void roundTripNormal()
  {
    final byte[] bytes = WearNavigationStateCodec.encode(WearNavigationState.normal());
    assertEquals(WearNavigationMode.NORMAL, WearNavigationStateCodec.decode(bytes).getMode());
  }

  @Test
  public void decodeNullOrEmptyReturnsNull()
  {
    assertNull(WearNavigationStateCodec.decode(null));
    assertNull(WearNavigationStateCodec.decode(new byte[0]));
  }

  @Test
  public void decodeVersionMismatchReturnsNull()
  {
    final byte[] future = "{\"version\":999,\"mode\":\"NAVIGATION\"}".getBytes(StandardCharsets.UTF_8);
    assertNull(WearNavigationStateCodec.decode(future));
  }

  @Test
  public void decodeUnknownModeReturnsNull()
  {
    final byte[] bogus =
        ("{\"version\":" + WearNavigationData.VERSION + ",\"mode\":\"BOGUS\"}").getBytes(StandardCharsets.UTF_8);
    assertNull(WearNavigationStateCodec.decode(bogus));
  }

  @Test
  public void decodeGarbageReturnsNull()
  {
    assertNull(WearNavigationStateCodec.decode("not json".getBytes(StandardCharsets.UTF_8)));
  }
}
