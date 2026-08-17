package app.organicmaps.sdk.util;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.function.IntSupplier;

final class ResourceIdCache
{
  private final ConcurrentMap<String, Integer> mIds = new ConcurrentHashMap<>();

  int get(String resourceName, IntSupplier resolver)
  {
    final Integer cachedId = mIds.get(resourceName);
    if (cachedId != null)
      return cachedId;

    // Do not call into Android's Resources while holding a cache lock. Concurrent first lookups may duplicate work,
    // but putIfAbsent makes all callers converge on one cached id.
    final int resolvedId = resolver.getAsInt();
    final Integer previousId = mIds.putIfAbsent(resourceName, resolvedId);
    return previousId == null ? resolvedId : previousId;
  }
}
