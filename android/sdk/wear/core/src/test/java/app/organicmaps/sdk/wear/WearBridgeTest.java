package app.organicmaps.sdk.wear;

import static org.junit.Assert.assertEquals;

import app.organicmaps.wear.protocol.WearNavigationMode;
import app.organicmaps.wear.protocol.WearNavigationState;
import org.junit.Test;

public class WearBridgeTest
{
  @Test
  public void publishesNavigationMode()
  {
    final RecordingPublisher publisher = new RecordingPublisher();
    WearBridge.register(publisher);

    WearBridge.publishNavigating(true);

    assertEquals(WearNavigationMode.NAVIGATION, publisher.mState.getMode());
  }

  @Test
  public void publishesNormalMode()
  {
    final RecordingPublisher publisher = new RecordingPublisher();
    WearBridge.register(publisher);

    WearBridge.publishNavigating(false);

    assertEquals(WearNavigationMode.NORMAL, publisher.mState.getMode());
  }

  @Test
  public void registrationReplacesPublisher()
  {
    final RecordingPublisher replaced = new RecordingPublisher();
    final RecordingPublisher current = new RecordingPublisher();
    WearBridge.register(replaced);
    WearBridge.register(current);

    WearBridge.publishNavigating(true);

    assertEquals(0, replaced.mPublishCount);
    assertEquals(1, current.mPublishCount);
  }

  private static final class RecordingPublisher implements WearNavigationPublisher
  {
    private WearNavigationState mState;
    private int mPublishCount;

    @Override
    public void publish(WearNavigationState state)
    {
      mState = state;
      ++mPublishCount;
    }
  }
}
