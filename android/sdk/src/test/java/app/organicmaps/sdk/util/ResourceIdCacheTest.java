package app.organicmaps.sdk.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;

public class ResourceIdCacheTest
{
  @Test
  public void get_cachesIdsByName()
  {
    final ResourceIdCache cache = new ResourceIdCache();
    final AtomicInteger resolutions = new AtomicInteger();

    assertEquals(1, cache.get("type.place.city", resolutions::incrementAndGet));
    assertEquals(1, cache.get("type.place.city", resolutions::incrementAndGet));
    assertEquals(2, cache.get("type.place.town", resolutions::incrementAndGet));
    assertEquals(2, resolutions.get());
  }

  @Test
  public void get_cachesMissingIds()
  {
    final ResourceIdCache cache = new ResourceIdCache();
    final AtomicInteger resolutions = new AtomicInteger();

    assertEquals(0, cache.get("missing", () -> {
      resolutions.incrementAndGet();
      return 0;
    }));
    assertEquals(0, cache.get("missing", () -> {
      resolutions.incrementAndGet();
      return 42;
    }));
    assertEquals(1, resolutions.get());
  }

  @Test
  public void get_doesNotCacheResolutionFailures()
  {
    final ResourceIdCache cache = new ResourceIdCache();
    final AtomicInteger resolutions = new AtomicInteger();

    assertThrows(IllegalStateException.class, () -> cache.get("type.place.city", () -> {
      resolutions.incrementAndGet();
      throw new IllegalStateException();
    }));
    assertEquals(7, cache.get("type.place.city", () -> {
      resolutions.incrementAndGet();
      return 7;
    }));
    assertEquals(2, resolutions.get());
  }

  @Test(timeout = 5000)
  public void get_concurrentResolutionsConvergeOnOneId() throws Exception
  {
    final int threadCount = 8;
    final ResourceIdCache cache = new ResourceIdCache();
    final AtomicInteger resolutions = new AtomicInteger();
    final CountDownLatch start = new CountDownLatch(1);
    final ExecutorService executor = Executors.newFixedThreadPool(threadCount);

    try
    {
      final List<Future<Integer>> results = new ArrayList<>();
      for (int i = 0; i < threadCount; ++i)
      {
        results.add(executor.submit(() -> {
          start.await();
          return cache.get("type.place.city", resolutions::incrementAndGet);
        }));
      }

      start.countDown();
      final int cachedId = results.get(0).get();
      for (Future<Integer> result : results)
        assertEquals(cachedId, result.get().intValue());

      assertTrue(resolutions.get() >= 1);
      assertTrue(resolutions.get() <= threadCount);
      final int resolutionCount = resolutions.get();
      assertEquals(cachedId, cache.get("type.place.city", resolutions::incrementAndGet));
      assertEquals(resolutionCount, resolutions.get());
    }
    finally
    {
      executor.shutdownNow();
    }
  }
}
