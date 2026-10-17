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
import android.os.Parcelable;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.ExecutorService;
import org.junit.Test;
import org.mockito.MockedStatic;

public class KmzKmlProcessorTest
{
  private static final String[] ACTIONS = {Intent.ACTION_VIEW, Intent.ACTION_SEND, Intent.ACTION_SEND_MULTIPLE};

  @Test
  public void unsupportedSchemesDoNotStartImport()
  {
    for (String action : ACTIONS)
    {
      for (String scheme : new String[] {"om", "ge0", "geo", "http", "https", "data", "android.resource", null})
        assertRejected(intentForAction(action, uriWithScheme(scheme)));
    }
  }

  @Test
  public void missingUriDoesNotStartImport()
  {
    for (String action : ACTIONS)
      assertRejected(intentWithAction(action));
  }

  @Test
  public void emptySharedListDoesNotStartImport()
  {
    assertRejected(sendMultipleIntent());
  }

  @Test
  public void invalidEntryRejectsEntireSharedList()
  {
    for (Parcelable invalid : new Parcelable[] {null, uriWithScheme("https"), mock(Intent.class)})
      assertRejected(sendMultipleIntent(uriWithScheme("content"), invalid));
  }

  @Test
  public void nonUriSharedStreamDoesNotStartImport()
  {
    final Intent intent = intentWithAction(Intent.ACTION_SEND);
    when(intent.getParcelableExtra(Intent.EXTRA_STREAM)).thenReturn(mock(Intent.class));
    assertRejected(intent);
  }

  @Test
  public void unrelatedActionsDoNotStartImport()
  {
    for (String action : new String[] {Intent.ACTION_EDIT, null})
      assertRejected(intentWithAction(action));
  }

  @Test
  public void localAndProviderFilesScheduleImportAndConsumeIntent()
  {
    for (String action : ACTIONS)
    {
      for (String scheme : new String[] {"content", "file"})
        assertImportScheduled(intentForAction(action, uriWithScheme(scheme)));
    }
  }

  @Test
  public void multipleLocalAndProviderFilesScheduleOneImport()
  {
    assertImportScheduled(sendMultipleIntent(uriWithScheme("content"), uriWithScheme("file")));
  }

  private static void assertRejected(Intent intent)
  {
    final MwmActivity activity = mock(MwmActivity.class);
    try (MockedStatic<ThreadPool> pool = mockStatic(ThreadPool.class))
    {
      assertFalse(new Factory.KmzKmlProcessor().process(intent, activity));
      verifyNoInteractions(activity);
      pool.verifyNoInteractions();
    }
  }

  private static void assertImportScheduled(Intent intent)
  {
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

  private static Intent intentWithAction(String action)
  {
    final Intent intent = mock(Intent.class);
    when(intent.getAction()).thenReturn(action);
    return intent;
  }

  private static Intent intentForAction(String action, Uri uri)
  {
    if (Intent.ACTION_SEND_MULTIPLE.equals(action))
      return sendMultipleIntent(uri);
    final Intent intent = intentWithAction(action);
    if (Intent.ACTION_VIEW.equals(action))
      when(intent.getData()).thenReturn(uri);
    else
      when(intent.getParcelableExtra(Intent.EXTRA_STREAM)).thenReturn(uri);
    return intent;
  }

  private static Intent sendMultipleIntent(Parcelable... streams)
  {
    final Intent intent = intentWithAction(Intent.ACTION_SEND_MULTIPLE);
    when(intent.getParcelableArrayListExtra(Intent.EXTRA_STREAM)).thenReturn(new ArrayList<>(Arrays.asList(streams)));
    return intent;
  }

  private static Uri uriWithScheme(String scheme)
  {
    final Uri uri = mock(Uri.class);
    when(uri.getScheme()).thenReturn(scheme);
    return uri;
  }
}
