package app.organicmaps;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Intent;
import org.junit.Test;

public class SplashActivityTest
{
  @Test
  public void intentRelaunchedFromHistoryIsMarkedConsumed()
  {
    final Intent intent = mock(Intent.class);
    when(intent.hasExtra(MwmActivity.EXTRA_CONSUMED)).thenReturn(false);
    when(intent.getFlags()).thenReturn(Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY);

    assertTrue(SplashActivity.markIntentConsumedIfRelaunchedFromHistory(intent));

    verify(intent).putExtra(MwmActivity.EXTRA_CONSUMED, true);
  }

  @Test
  public void freshIntentIsLeftUntouched()
  {
    final Intent intent = mock(Intent.class);
    when(intent.hasExtra(MwmActivity.EXTRA_CONSUMED)).thenReturn(false);
    when(intent.getFlags()).thenReturn(Intent.FLAG_GRANT_READ_URI_PERMISSION);

    assertFalse(SplashActivity.markIntentConsumedIfRelaunchedFromHistory(intent));

    verify(intent, never()).putExtra(anyString(), anyBoolean());
  }

  /**
   * An activity restarting the core forwards what it really knows about the intent, so the history
   * flag must not overwrite a file that has been shared but not imported yet.
   */
  @Test
  public void forwardedStateWinsOverHistoryFlag()
  {
    final Intent intent = mock(Intent.class);
    when(intent.hasExtra(MwmActivity.EXTRA_CONSUMED)).thenReturn(true);
    when(intent.getFlags()).thenReturn(Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY);

    assertFalse(SplashActivity.markIntentConsumedIfRelaunchedFromHistory(intent));

    verify(intent, never()).putExtra(anyString(), anyBoolean());
  }
}
