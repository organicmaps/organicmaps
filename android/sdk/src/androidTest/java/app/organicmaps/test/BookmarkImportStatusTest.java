package app.organicmaps.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

public class BookmarkImportStatusTest
{
  private static final String MALFORMED_KML = "<kml><Document></Placemark></Document></kml>";

  private static OrganicMaps sSdk;

  private final BlockingQueue<String> mEvents = new LinkedBlockingQueue<>();
  private final Set<Long> mExistingCategories = new HashSet<>();
  private final BookmarkManager.BookmarksLoadingListener mListener = new BookmarkManager.BookmarksLoadingListener() {
    @Override
    public void onBookmarksFileImportSuccessful()
    {
      mEvents.add("success");
    }

    @Override
    public void onBookmarksFileImportFailed()
    {
      mEvents.add("failure");
    }
  };
  private File mDirectory;

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
    mDirectory = new File(context.getCacheDir(), "bookmark-import-status-test-" + UUID.randomUUID());
    assertTrue(mDirectory.mkdirs());
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
    for (File file : listFiles(mDirectory))
      assertTrue(file.delete());
    assertTrue(mDirectory.delete());
  }

  @Test
  public void validFileReportsSuccessAndDeletesTemporaryCopy() throws Exception
  {
    File file = write("valid.kml", kml("Valid"));
    load(file, true);
    assertEquals("success", nextEvent());
    assertFalse(file.exists());
    assertImportedCategories(1);
  }

  @Test
  public void corruptFileReportsFailureAndKeepsTheSource() throws Exception
  {
    File file = write("corrupt.kml", MALFORMED_KML);
    load(file, false);
    assertEquals("failure", nextEvent());
    assertTrue(file.exists());
    assertImportedCategories(0);
  }

  @Test
  public void partiallyImportedKmzReportsFailure() throws Exception
  {
    File file = new File(mDirectory, "partial.kmz");
    try (ZipOutputStream zip = new ZipOutputStream(new FileOutputStream(file)))
    {
      zip.putNextEntry(new ZipEntry("good.kml"));
      zip.write(kml("Good").getBytes(StandardCharsets.UTF_8));
      zip.putNextEntry(new ZipEntry("bad.kml"));
      zip.write(MALFORMED_KML.getBytes(StandardCharsets.UTF_8));
    }
    load(file, true);
    assertEquals("failure", nextEvent());
    assertFalse(file.exists());
    assertImportedCategories(1);
  }

  private static String kml(String name)
  {
    String placemark = "<Placemark><Point><coordinates>2,2</coordinates></Point></Placemark>";
    return "<kml><Document><name>" + name + "</name>" + placemark + "</Document></kml>";
  }

  private File write(String name, String content) throws IOException
  {
    File file = new File(mDirectory, name);
    try (FileOutputStream stream = new FileOutputStream(file))
    {
      stream.write(content.getBytes(StandardCharsets.UTF_8));
    }
    return file;
  }

  private static void load(File file, boolean isTemporaryFile)
  {
    runOnMain(() -> BookmarkManager.INSTANCE.loadBookmarksFile(file.getAbsolutePath(), isTemporaryFile));
  }

  private String nextEvent() throws InterruptedException
  {
    String event = mEvents.poll(30, TimeUnit.SECONDS);
    assertNotNull("Bookmark import did not report a result", event);
    runOnMain(() -> assertTrue("Unexpected additional import feedback: " + mEvents, mEvents.isEmpty()));
    return event;
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

  private static void runOnMain(Runnable task)
  {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(task);
  }
}
