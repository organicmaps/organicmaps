package app.organicmaps.sdk.util;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

final class ResourceIdCache
{
  @FunctionalInterface
  interface Resolver {
    int resolve();
  }

  private final ConcurrentMap<String, ConcurrentMap<String, Integer>> mIds = new ConcurrentHashMap<>();

  int get(String packageName, String resourceName, Resolver resolver)
  {
    ConcurrentMap<String, Integer> packageIds = mIds.get(packageName);
    if (packageIds == null)
    {
      final ConcurrentMap<String, Integer> newPackageIds = new ConcurrentHashMap<>();
      final ConcurrentMap<String, Integer> previousPackageIds = mIds.putIfAbsent(packageName, newPackageIds);
      packageIds = previousPackageIds == null ? newPackageIds : previousPackageIds;
    }

    final Integer cachedId = packageIds.get(resourceName);
    if (cachedId != null)
      return cachedId;

    // Do not call into Android's Resources while holding a cache lock. Concurrent first lookups may duplicate work,
    // but putIfAbsent makes all callers converge on one cached id.
    final int resolvedId = resolver.resolve();
    final Integer previousId = packageIds.putIfAbsent(resourceName, resolvedId);
    return previousId == null ? resolvedId : previousId;
  }
}
