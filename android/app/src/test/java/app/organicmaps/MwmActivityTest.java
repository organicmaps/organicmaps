package app.organicmaps;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.os.Bundle;
import org.junit.Test;
import org.mockito.InOrder;

public class MwmActivityTest
{
  @Test
  public void unmarkedOrMissingIntentIsNotConsumed()
  {
    assertFalse(MwmActivity.isIntentConsumed(null, null));
    assertFalse(MwmActivity.isIntentConsumed(null, mock(Intent.class)));
  }

  @Test
  public void consumedStateSurvivesCoreRestart()
  {
    final Intent intent = mock(Intent.class);
    when(intent.hasExtra(MwmActivity.EXTRA_CONSUMED)).thenReturn(true);
    when(intent.getBooleanExtra(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(true);

    assertTrue(MwmActivity.isIntentConsumed(null, intent));
  }

  @Test
  public void relaunchFromRecentsIsConsumed()
  {
    final Intent intent = mock(Intent.class);
    when(intent.getFlags()).thenReturn(Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY);

    assertTrue(MwmActivity.isIntentConsumed(null, intent));
  }

  @Test
  public void explicitMarkWinsOverRecents()
  {
    final Intent intent = mock(Intent.class);
    when(intent.hasExtra(MwmActivity.EXTRA_CONSUMED)).thenReturn(true);
    when(intent.getBooleanExtra(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(false);
    when(intent.getFlags()).thenReturn(Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY);

    assertFalse(MwmActivity.isIntentConsumed(null, intent));
  }

  /**
   * The saved state belongs to this very instance, so it is more recent than anything the intent was
   * marked with back when the instance was created.
   */
  @Test
  public void savedStateWinsOverIntent()
  {
    final Bundle savedInstanceState = mock(Bundle.class);
    when(savedInstanceState.getBoolean(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(false);
    final Intent intent = mock(Intent.class);
    when(intent.getBooleanExtra(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(true);

    assertFalse(MwmActivity.isIntentConsumed(savedInstanceState, intent));
  }

  @Test
  public void pendingBatterySettingsRecordingSurvivesCoreRestart()
  {
    final Bundle state = mock(Bundle.class);
    when(state.getBoolean(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(true);
    when(state.getBoolean(MwmActivity.EXTRA_START_TRACK_RECORDING, false)).thenReturn(true);
    assertRecordingContinuesAfterCoreRestart(state);
  }

  @Test
  public void pendingLocationPermissionRecordingSurvivesCoreRestart()
  {
    final Bundle state = mock(Bundle.class);
    when(state.getBoolean(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(true);
    when(state.getBoolean(MwmActivity.LOCATION_PERMISSION_REQUESTED_FOR_RECORDING, false)).thenReturn(true);
    assertRecordingContinuesAfterCoreRestart(state);
  }

  private static void assertRecordingContinuesAfterCoreRestart(Bundle state)
  {
    final Intent intent = mock(Intent.class);
    final MwmActivity activity = mock(MwmActivity.class, CALLS_REAL_METHODS);
    activity.prepareIntentForCoreRestart(intent, state);

    // A fresh recording request must override the consumed mark on the original launch intent.
    final InOrder order = inOrder(intent);
    order.verify(intent).putExtra(MwmActivity.EXTRA_CONSUMED, true);
    order.verify(intent).putExtra(MwmActivity.EXTRA_START_TRACK_RECORDING, true);
    order.verify(intent).putExtra(MwmActivity.EXTRA_CONSUMED, false);
  }

  @Test
  public void completedRecordingRequestIsNotReplayedAfterCoreRestart()
  {
    final Bundle state = mock(Bundle.class);
    when(state.getBoolean(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(true);
    final Intent intent = mock(Intent.class);
    final MwmActivity activity = mock(MwmActivity.class, CALLS_REAL_METHODS);
    activity.prepareIntentForCoreRestart(intent, state);

    verify(intent).putExtra(MwmActivity.EXTRA_CONSUMED, true);
    verify(intent, never()).putExtra(MwmActivity.EXTRA_START_TRACK_RECORDING, true);
    verify(intent, never()).putExtra(MwmActivity.EXTRA_CONSUMED, false);
  }

  @Test
  public void missingStateLeavesCoreRestartIntentUnchanged()
  {
    final Intent intent = mock(Intent.class);
    final MwmActivity activity = mock(MwmActivity.class, CALLS_REAL_METHODS);
    activity.prepareIntentForCoreRestart(intent, null);

    verifyNoInteractions(intent);
  }
}
