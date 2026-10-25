package app.organicmaps.sdk.car.renderer;

import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import androidx.core.util.Consumer;
import app.organicmaps.sdk.widgets.speedlimit.CombinedSpeedViewController;
import org.junit.Test;

public class SurfaceCallbackTest
{
  @SuppressWarnings("unchecked")
  private final Consumer<CombinedSpeedViewController> mListener = mock(Consumer.class);

  @Test
  public void deliversExistingController()
  {
    final SurfaceCallback callback = mock(SurfaceCallback.class);
    final CombinedSpeedViewController controller = mock(CombinedSpeedViewController.class);
    when(callback.getSpeedLimitController()).thenReturn(controller);
    doCallRealMethod().when(callback).setSpeedLimitControllerListener(mListener);

    callback.setSpeedLimitControllerListener(mListener);

    verify(mListener).accept(controller);
  }

  @Test
  public void waitsForPresentationWhenControllerIsUnavailable()
  {
    final SurfaceCallback callback = mock(SurfaceCallback.class);
    doCallRealMethod().when(callback).setSpeedLimitControllerListener(mListener);

    callback.setSpeedLimitControllerListener(mListener);

    verifyNoInteractions(mListener);
  }

  @Test
  public void removingListenerDoesNotNotifyIt()
  {
    final SurfaceCallback callback = mock(SurfaceCallback.class);
    when(callback.getSpeedLimitController()).thenReturn(mock(CombinedSpeedViewController.class));
    doCallRealMethod().when(callback).setSpeedLimitControllerListener(mListener);
    doCallRealMethod().when(callback).setSpeedLimitControllerListener(null);

    when(callback.getSpeedLimitController()).thenReturn(null);
    callback.setSpeedLimitControllerListener(mListener);
    when(callback.getSpeedLimitController()).thenReturn(mock(CombinedSpeedViewController.class));
    callback.setSpeedLimitControllerListener(null);

    verifyNoInteractions(mListener);
  }
}
