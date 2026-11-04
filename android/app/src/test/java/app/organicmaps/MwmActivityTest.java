package app.organicmaps;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.os.Bundle;
import java.util.HashMap;
import java.util.Map;
import org.junit.Test;

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
  public void pendingLocationPermissionRecordingSurvivesCoreRestart()
  {
    final Bundle state = mock(Bundle.class);
    when(state.getBoolean(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(true);
    when(state.getBoolean(MwmActivity.LOCATION_PERMISSION_REQUESTED_FOR_RECORDING, false)).thenReturn(true);
    final Intent intent = statefulIntent();
    intent.putExtra(MwmActivity.EXTRA_CONSUMED, true);
    final MwmActivity activity = mock(MwmActivity.class, CALLS_REAL_METHODS);
    activity.prepareIntentForCoreRestart(intent, state);

    assertTrue(intent.getBooleanExtra(MwmActivity.EXTRA_START_TRACK_RECORDING, false));
    assertFalse(MwmActivity.isIntentConsumed(null, intent));
  }

  @Test
  public void completedRecordingRequestIsNotReplayedAfterCoreRestart()
  {
    final Bundle state = mock(Bundle.class);
    when(state.getBoolean(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(true);
    final Intent intent = statefulIntent();
    intent.putExtra(MwmActivity.EXTRA_START_TRACK_RECORDING, true);
    final MwmActivity activity = mock(MwmActivity.class, CALLS_REAL_METHODS);
    activity.prepareIntentForCoreRestart(intent, state);

    assertTrue(MwmActivity.isIntentConsumed(null, intent));
  }

  @Test
  public void missingStateLeavesCoreRestartIntentUnchanged()
  {
    final Intent intent = mock(Intent.class);
    final MwmActivity activity = mock(MwmActivity.class, CALLS_REAL_METHODS);
    activity.prepareIntentForCoreRestart(intent, null);

    verifyNoInteractions(intent);
  }

  private static Intent statefulIntent()
  {
    final Map<String, Boolean> extras = new HashMap<>();
    final Intent intent = mock(Intent.class);
    doAnswer(call -> {
      extras.put(call.getArgument(0), call.getArgument(1));
      return intent;
    })
        .when(intent)
        .putExtra(anyString(), anyBoolean());
    when(intent.hasExtra(anyString())).thenAnswer(call -> extras.containsKey(call.getArgument(0)));
    when(intent.getBooleanExtra(anyString(), anyBoolean()))
        .thenAnswer(call -> extras.getOrDefault(call.getArgument(0), call.getArgument(1)));
    return intent;
  }
}
