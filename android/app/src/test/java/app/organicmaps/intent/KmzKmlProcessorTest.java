package app.organicmaps.intent;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.net.Uri;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import java.io.File;
import java.util.concurrent.ExecutorService;
import org.junit.Test;
import org.mockito.MockedStatic;

public class KmzKmlProcessorTest
{
  @Test
  public void unsupportedSchemesDoNotStartImportEvenWithBookmarkMimeType()
  {
    for (String scheme : new String[] {"om", "ge0", "geo", "http", "https", "data", null})
    {
      final Intent intent = viewIntent(scheme);
      when(intent.getType()).thenReturn("application/vnd.google-earth.kml+xml");
      final MwmActivity activity = mock(MwmActivity.class);
      assertFalse(new Factory.KmzKmlProcessor().process(intent, activity));
      verifyNoInteractions(activity);
    }
  }

  @Test
  public void missingUriDoesNotStartImport()
  {
    final Intent intent = mock(Intent.class);
    when(intent.getAction()).thenReturn(Intent.ACTION_VIEW);
    final MwmActivity activity = mock(MwmActivity.class);
    assertFalse(new Factory.KmzKmlProcessor().process(intent, activity));
    verifyNoInteractions(activity);
  }

  @Test
  public void localAndProviderFilesScheduleImportAndConsumeIntent()
  {
    for (String scheme : new String[] {"content", "file"})
    {
      final Intent intent = viewIntent(scheme);
      final MwmActivity activity = mock(MwmActivity.class);
      final MwmApplication app = mock(MwmApplication.class);
      when(activity.getApplicationContext()).thenReturn(app);
      when(app.getCacheDir()).thenReturn(new File("test-cache"));
      final ExecutorService storage = mock(ExecutorService.class);
      try (MockedStatic<ThreadPool> pool = mockStatic(ThreadPool.class))
      {
        pool.when(ThreadPool::getStorage).thenReturn(storage);
        assertTrue(new Factory.KmzKmlProcessor().process(intent, activity));
        verify(storage).execute(any(Runnable.class));
      }
    }
  }

  private static Intent viewIntent(String scheme)
  {
    final Intent intent = mock(Intent.class);
    final Uri uri = mock(Uri.class);
    when(intent.getAction()).thenReturn(Intent.ACTION_VIEW);
    when(intent.getData()).thenReturn(uri);
    when(uri.getScheme()).thenReturn(scheme);
    return intent;
  }
}
