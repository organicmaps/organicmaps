package app.organicmaps;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.os.Bundle;
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
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
    when(intent.getBooleanExtra(MwmActivity.EXTRA_CONSUMED, false)).thenReturn(true);

    assertTrue(MwmActivity.isIntentConsumed(null, intent));
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
  public void missingBookmarkDoesNotCrash()
  {
    final Intent intent = mock(Intent.class);
    when(intent.getLongExtra(MwmActivity.EXTRA_BOOKMARK_ID, -1)).thenReturn(999999L);
    when(intent.getLongExtra(MwmActivity.EXTRA_TRACK_ID, -1)).thenReturn(-1L);
    when(intent.getLongExtra(MwmActivity.EXTRA_CATEGORY_ID, -1)).thenReturn(-1L);

    final BookmarkManager bm = mock(BookmarkManager.class);
    when(bm.getBookmarkInfo(999999L)).thenReturn(null);

    assertTrue(MwmActivity.showBookmarkOrTrackFromIntent(intent, bm));
    verify(bm, never()).showBookmarkOnMap(anyLong());
  }

  @Test
  public void missingTrackDoesNotCrash()
  {
    final Intent intent = mock(Intent.class);
    when(intent.getLongExtra(MwmActivity.EXTRA_BOOKMARK_ID, -1)).thenReturn(-1L);
    when(intent.getLongExtra(MwmActivity.EXTRA_TRACK_ID, -1)).thenReturn(888888L);
    when(intent.getLongExtra(MwmActivity.EXTRA_CATEGORY_ID, -1)).thenReturn(-1L);

    final BookmarkManager bm = mock(BookmarkManager.class);
    when(bm.hasTrack(888888L)).thenReturn(false);

    assertTrue(MwmActivity.showBookmarkOrTrackFromIntent(intent, bm));
  }

  @Test
  public void validBookmarkIsShown()
  {
    final Intent intent = mock(Intent.class);
    when(intent.getLongExtra(MwmActivity.EXTRA_BOOKMARK_ID, -1)).thenReturn(123L);
    when(intent.getLongExtra(MwmActivity.EXTRA_TRACK_ID, -1)).thenReturn(-1L);
    when(intent.getLongExtra(MwmActivity.EXTRA_CATEGORY_ID, -1)).thenReturn(-1L);

    final BookmarkManager bm = mock(BookmarkManager.class);
    final BookmarkInfo info = mock(BookmarkInfo.class);
    when(bm.getBookmarkInfo(123L)).thenReturn(info);

    assertTrue(MwmActivity.showBookmarkOrTrackFromIntent(intent, bm));
    verify(bm).showBookmarkOnMap(123L);
  }

  @Test
  public void intentWithoutBookmarkOrTrackReturnsFalse()
  {
    final Intent intent = mock(Intent.class);
    when(intent.getLongExtra(MwmActivity.EXTRA_BOOKMARK_ID, -1)).thenReturn(-1L);
    when(intent.getLongExtra(MwmActivity.EXTRA_TRACK_ID, -1)).thenReturn(-1L);
    when(intent.getLongExtra(MwmActivity.EXTRA_CATEGORY_ID, -1)).thenReturn(-1L);

    final BookmarkManager bm = mock(BookmarkManager.class);

    assertFalse(MwmActivity.showBookmarkOrTrackFromIntent(intent, bm));
  }
}
