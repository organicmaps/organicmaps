package app.organicmaps.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.FutureTask;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

public class BookmarkImportTest
{
  private static OrganicMaps sSdk;
  private static final Uri NULL_STREAM = source("null/route.gpx");
  private static final Uri FIRST_SOURCE = source("valid/first/route.gpx");
  private static final Uri SECOND_SOURCE = source("valid/second/route.gpx");
  private static final Uri MALFORMED_SOURCE = source("malformed/route.gpx");

  private record ImportEvent(String message, int categoryCount)
  {
  }

  private final BlockingQueue<ImportEvent> mEvents = new LinkedBlockingQueue<>();
  private final Set<Long> mExistingCategories = new HashSet<>();
  private final BookmarkManager.BookmarksLoadingListener mListener = new BookmarkManager.BookmarksLoadingListener() {
    @Override
    public void onBookmarksFileDownloadFailed(Uri uri, String error)
    {
      recordEvent("read error: " + uri);
    }

    @Override
    public void onBookmarksFileUnsupported(Uri uri)
    {
      recordEvent("unsupported: " + uri);
    }

    @Override
    public void onBookmarksFileImportSuccessful()
    {
      recordEvent("success");
    }

    @Override
    public void onBookmarksFileImportFailed()
    {
      recordEvent("parse error");
    }
  };
  private File mImportDirectory;

  @BeforeClass
  public static void initializeCore() throws Exception
  {
    CountDownLatch loaded = new CountDownLatch(1);
    AtomicReference<IOException> failure = new AtomicReference<>();
    BookmarkManager.BookmarksLoadingListener listener = new BookmarkManager.BookmarksLoadingListener() {
      @Override
      public void onBookmarksLoadingFinished()
      {
        loaded.countDown();
      }
    };
    runOnMain(() -> {
      Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
      sSdk = new OrganicMaps(context, "test", context.getPackageName(), 1, "test");
      BookmarkManager.INSTANCE.addLoadingListener(listener);
      try
      {
        sSdk.init(() -> {});
      }
      catch (IOException e)
      {
        failure.set(e);
      }
    });
    assertNull(failure.get());
    assertTrue("Initial bookmark loading did not finish", loaded.await(30, TimeUnit.SECONDS));
    runOnMain(() -> BookmarkManager.INSTANCE.removeLoadingListener(listener));
  }

  @Before
  public void setUp()
  {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    mImportDirectory = new File(context.getCacheDir(), "bookmark-import-test-" + UUID.randomUUID());
    assertTrue(mImportDirectory.mkdirs());
    runOnMain(() -> {
      for (BookmarkCategory category : BookmarkManager.INSTANCE.getCategories())
        mExistingCategories.add(category.getId());
      BookmarkManager.INSTANCE.addLoadingListener(mListener);
    });
  }

  @After
  public void tearDown()
  {
    runOnMain(() -> {
      BookmarkManager.INSTANCE.removeLoadingListener(mListener);
      for (BookmarkCategory category : new ArrayList<>(BookmarkManager.INSTANCE.getCategories()))
        if (!mExistingCategories.contains(category.getId()))
          BookmarkManager.INSTANCE.deleteCategory(category.getId());
    });
    deleteRecursively(mImportDirectory);
  }

  @Test
  public void nullStreamReportsOneReadErrorAndLeavesNoStagingFiles() throws Exception
  {
    ContentResolver resolver = InstrumentationRegistry.getInstrumentation().getContext().getContentResolver();
    assertNull(resolver.openInputStream(NULL_STREAM));

    assertTrue(prepareBatches(List.of(List.of(NULL_STREAM))).isEmpty());
    assertEquals("read error: " + NULL_STREAM, nextEvent(0));
    assertStagingCleanedUp(List.of());
    assertImportedCategories(0);
    assertNoMoreEvents();
  }

  @Test
  public void mixedBatchKeepsPreparationFailureUntilNativeCompletion() throws Exception
  {
    List<File> stagedFiles = prepareBatches(List.of(List.of(NULL_STREAM, FIRST_SOURCE)));
    assertEquals(1, stagedFiles.size());
    assertEquals("read error: " + NULL_STREAM, nextEvent(1));
    assertStagingCleanedUp(stagedFiles);
    assertImportedCategories(1);
    assertNoMoreEvents();
  }

  @Test
  public void identicalBasenamesUseDistinctPathsAndAreBothCleanedUp() throws Exception
  {
    List<File> stagedFiles = prepareBatches(List.of(List.of(FIRST_SOURCE, SECOND_SOURCE)));
    assertEquals(2, stagedFiles.size());
    assertEquals(stagedFiles.get(0).getName(), stagedFiles.get(1).getName());
    assertNotEquals(stagedFiles.get(0).getAbsolutePath(), stagedFiles.get(1).getAbsolutePath());
    assertEquals("success", nextEvent(2));
    assertStagingCleanedUp(stagedFiles);
    assertImportedCategories(2);
    assertNoMoreEvents();
  }

  @Test
  public void queuedBatchesConsumeTheirOwnFeedbackOnceInOrder() throws Exception
  {
    List<File> stagedFiles = prepareBatches(List.of(List.of(NULL_STREAM, FIRST_SOURCE), List.of(SECOND_SOURCE)));
    assertEquals(2, stagedFiles.size());
    assertEquals("read error: " + NULL_STREAM, nextEvent(1));
    assertEquals("success", nextEvent(2));
    assertStagingCleanedUp(stagedFiles);
    assertImportedCategories(2);
    assertNoMoreEvents();
  }

  @Test
  public void nativeParseFailureIsReportedBeforeTheNextQueuedBatch() throws Exception
  {
    List<File> stagedFiles = prepareBatches(List.of(List.of(MALFORMED_SOURCE), List.of(FIRST_SOURCE)));
    assertEquals(2, stagedFiles.size());
    assertEquals("parse error", nextEvent(0));
    assertEquals("success", nextEvent(1));
    assertStagingCleanedUp(stagedFiles);
    assertImportedCategories(1);
    assertNoMoreEvents();
  }

  private List<File> prepareBatches(List<List<Uri>> batches)
  {
    List<File> stagedFiles = new ArrayList<>();
    ContentResolver resolver = InstrumentationRegistry.getInstrumentation().getContext().getContentResolver();
    // Resolve the provider before preparation temporarily holds the main thread.
    assertEquals("application/gpx+xml", resolver.getType(FIRST_SOURCE));
    FutureTask<Void> preparation = new FutureTask<>(() -> {
      for (List<Uri> batch : batches)
        BookmarkManager.INSTANCE.importBookmarksFiles(resolver, batch, mImportDirectory);
      return null;
    });
    runOnMain(() -> {
      // Hold native submission until every batch is prepared, making staging and FIFO assertions deterministic.
      new Thread(preparation, "Bookmark import test").start();
      try
      {
        preparation.get(10, TimeUnit.SECONDS);
      }
      catch (Exception e)
      {
        throw new AssertionError(e);
      }
      for (File directory : listFiles(mImportDirectory))
        stagedFiles.addAll(List.of(listFiles(directory)));
      assertTrue("Preparation must not notify before the queued UI callback", mEvents.isEmpty());
    });
    return stagedFiles;
  }

  private void recordEvent(String message)
  {
    mEvents.add(new ImportEvent(message, BookmarkManager.INSTANCE.getCategoriesCount()));
  }

  private String nextEvent(int importedCategories) throws InterruptedException
  {
    ImportEvent event = mEvents.poll(30, TimeUnit.SECONDS);
    assertNotNull("Bookmark import did not report a result", event);
    assertEquals("Feedback \"" + event.message() + "\" must wait for native import",
                 mExistingCategories.size() + importedCategories, event.categoryCount());
    return event.message();
  }

  private void assertNoMoreEvents()
  {
    runOnMain(() -> assertTrue("Unexpected additional import feedback: " + mEvents, mEvents.isEmpty()));
  }

  private void assertStagingCleanedUp(List<File> stagedFiles)
  {
    for (File file : stagedFiles)
    {
      assertFalse(file.exists());
      assertFalse(file.getParentFile().exists());
    }
    assertEquals(0, listFiles(mImportDirectory).length);
  }

  private void assertImportedCategories(int count)
  {
    runOnMain(() -> assertEquals(mExistingCategories.size() + count, BookmarkManager.INSTANCE.getCategoriesCount()));
  }

  private static File[] listFiles(File directory)
  {
    File[] files = directory.listFiles();
    assertNotNull("Could not list " + directory, files);
    return files;
  }

  private static void deleteRecursively(File file)
  {
    if (file.isDirectory())
      for (File child : listFiles(file))
        deleteRecursively(child);
    assertTrue("Could not delete " + file, file.delete());
  }

  private static Uri source(String path)
  {
    return Uri.parse("content://app.organicmaps.sdk.test.bookmark-import/" + path);
  }

  private static void runOnMain(Runnable task)
  {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(task);
  }
}
