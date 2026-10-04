package app.organicmaps.search;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.search.SearchEngine.ContactAddressListener;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.Language;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public enum ContactMapManager implements ContactAddressListener
{
  INSTANCE;

  private static final int MAX_CONCURRENT_REQUESTS = 1;
  private static final long MARK_UPDATE_DELAY_MS = 100;

  static final class ResolvedAddress
  {
    final double lat;
    final double lon;
    final boolean estimated;

    ResolvedAddress(double lat, double lon, boolean estimated)
    {
      this.lat = lat;
      this.lon = lon;
      this.estimated = estimated;
    }
  }

  private static final class ContactMark
  {
    @NonNull
    final ResolvedAddress coordinate;
    @NonNull
    final Set<String> names = new LinkedHashSet<>();

    ContactMark(@NonNull ResolvedAddress coordinate)
    {
      this.coordinate = coordinate;
    }
  }

  private static final class PendingAddress
  {
    @NonNull
    final String key;
    @NonNull
    final List<String> contextTokens;
    @NonNull
    final List<ContactAddress.SearchQuery> queries;
    final long generation;

    PendingAddress(@NonNull String key, @NonNull List<String> contextTokens,
                   @NonNull List<ContactAddress.SearchQuery> queries, long generation)
    {
      this.key = key;
      this.contextTokens = contextTokens;
      this.queries = queries;
      this.generation = generation;
    }
  }

  private final Map<String, ResolvedAddress> mCache = new HashMap<>();
  private final Map<String, Set<String>> mNames = new HashMap<>();
  private final Set<String> mFailed = new HashSet<>();
  private final ArrayDeque<PendingAddress> mQueue = new ArrayDeque<>();
  private final Map<Long, PendingAddress> mRequests = new HashMap<>();
  private final Set<String> mVisibleKeys = new HashSet<>();
  private final Handler mMainHandler = new Handler(Looper.getMainLooper());
  private final Runnable mUpdateMarks = this::updateMarks;
  private final Runnable mContactsChanged = this::onContactsChanged;
  private ContactAddressSearch mContactSearch;
  private ContactLocationCache mPersistentCache;
  private Context mContext;
  private String mLocale = "en";
  private long mGeneration;
  private final Map<String, PendingAddress> mAddresses = new LinkedHashMap<>();
  private List<String> mViewportRegionTokens = List.of();
  private int mViewportScale;
  private final Runnable mResolveViewport = this::resolveViewport;
  private double[] mMarkLatitudes = {};
  private double[] mMarkLongitudes = {};
  @Nullable
  private String[] mMarkNames;
  private boolean[] mMarkEstimated = {};

  @MainThread
  public void refresh(@NonNull Context context)
  {
    mMarkNames = null;
    cancelRequests();
    mMainHandler.removeCallbacks(mUpdateMarks);
    mContext = context.getApplicationContext();
    ++mGeneration;
    mQueue.clear();
    mRequests.clear();
    mAddresses.clear();
    mMainHandler.removeCallbacks(mResolveViewport);
    mVisibleKeys.clear();
    mNames.clear();
    mFailed.clear();
    mCache.clear();

    if (!isEnabled(mContext))
    {
      SearchEngine.INSTANCE.setContactViewportListener(null);
      ContactAddressSearch.shutdown();
      mContactSearch = null;
      if (mPersistentCache == null)
        mPersistentCache = new ContactLocationCache(mContext);
      mPersistentCache.clear();
      updateMarks();
      return;
    }

    mLocale = Language.getKeyboardLocale(mContext);
    if (mContactSearch == null)
    {
      mContactSearch = ContactAddressSearch.getInstance(mContext);
      mContactSearch.addContactsChangedListener(mContactsChanged);
    }
    if (mPersistentCache == null)
      mPersistentCache = new ContactLocationCache(mContext);
    final long generation = mGeneration;
    mContactSearch.loadAll((ignored, addresses) -> onAddressesLoaded(generation, addresses));
    SearchEngine.INSTANCE.setContactViewportListener(this::onViewportChanged);
  }

  @MainThread
  public void refreshIfAvailabilityChanged(@NonNull Context context)
  {
    if (mContext != null && isEnabled(context) != (mContactSearch != null))
      refresh(context);
  }

  @MainThread
  private void onContactsChanged()
  {
    if (mPersistentCache != null)
      mPersistentCache.clear();
    if (mContext != null)
      refresh(mContext);
  }

  private static boolean isEnabled(@NonNull Context context)
  {
    return Config.isContactSearchEnabled()
 && ContextCompat.checkSelfPermission(context, Manifest.permission.READ_CONTACTS) == PackageManager.PERMISSION_GRANTED;
  }

  @MainThread
  private void onAddressesLoaded(long generation, @NonNull List<ContactAddress> addresses)
  {
    if (generation != mGeneration || mContext == null || !isEnabled(mContext))
      return;

    final Map<String, PendingAddress> unique = new LinkedHashMap<>();
    for (ContactAddress address : addresses)
    {
      final List<ContactAddress.SearchQuery> searchQueries = address.getSearchQueries();
      if (!searchQueries.isEmpty())
      {
        final String key = normalizeKey(address);
        unique.putIfAbsent(key, new PendingAddress(key, address.contextTokens, searchQueries, generation));
        mNames.computeIfAbsent(key, ignored -> new LinkedHashSet<>()).add(address.name);
      }
    }

    mVisibleKeys.addAll(unique.keySet());
    mPersistentCache.retain(unique.keySet());
    for (PendingAddress address : unique.values())
    {
      if (!mCache.containsKey(address.key))
      {
        final ContactLocationCache.Entry cached = mPersistentCache.get(address.key);
        if (cached != null)
          mCache.put(address.key, new ResolvedAddress(cached.lat, cached.lon, cached.estimated));
      }
    }
    mAddresses.putAll(unique);
    updateMarks();
    resolveViewport();
  }

  @MainThread
  private void resolveNext()
  {
    if (mViewportScale < 16)
      return;
    while (mRequests.size() < MAX_CONCURRENT_REQUESTS)
    {
      final PendingAddress address = mQueue.poll();
      if (address == null)
        return;
      if (address.generation != mGeneration || mCache.containsKey(address.key))
        continue;
      final long requestId = SearchEngine.INSTANCE.resolveContactAddress(
          address.queries.stream().map(query -> query.query).toArray(String[] ::new),
          address.queries.stream().map(query -> query.expectedStreet).toArray(String[] ::new), mLocale, true, this);
      mRequests.put(requestId, address);
    }
  }

  @Override
  @MainThread
  public void onContactAddressResolved(long requestId, boolean found, double lat, double lon, boolean estimated)
  {
    if (mContext != null && !isEnabled(mContext))
    {
      refresh(mContext);
      return;
    }
    final PendingAddress address = mRequests.remove(requestId);
    if (address == null || address.generation != mGeneration)
    {
      resolveNext();
      return;
    }

    if (found)
    {
      final ResolvedAddress resolved = new ResolvedAddress(lat, lon, estimated);
      mCache.put(address.key, resolved);
      mPersistentCache.put(address.key, new ContactLocationCache.Entry(lat, lon, estimated));
      scheduleMarksUpdate();
    }
    else
      mFailed.add(address.key);
    resolveNext();
  }

  @MainThread
  void recordResolved(@NonNull ContactAddress contactAddress, double lat, double lon, boolean estimated)
  {
    if (mContext == null || !isEnabled(mContext))
      return;
    final String key = normalizeKey(contactAddress);
    final ResolvedAddress resolved = new ResolvedAddress(lat, lon, estimated);
    mCache.put(key, resolved);
    mQueue.removeIf(address -> address.key.equals(key));
    mRequests.entrySet().removeIf(request -> {
      if (!request.getValue().key.equals(key))
        return false;
      SearchEngine.INSTANCE.cancelContactAddressResolution(request.getKey());
      return true;
    });
    if (mPersistentCache != null)
      mPersistentCache.put(key, new ContactLocationCache.Entry(lat, lon, estimated));
    mFailed.remove(key);
    mVisibleKeys.add(key);
    mNames.computeIfAbsent(key, ignored -> new LinkedHashSet<>()).add(contactAddress.name);
    scheduleMarksUpdate();
    resolveNext();
  }

  @Nullable
  ResolvedAddress getResolved(@NonNull ContactAddress contactAddress)
  {
    if (mContext == null || !isEnabled(mContext))
      return null;
    final String key = normalizeKey(contactAddress);
    ResolvedAddress resolved = mCache.get(key);
    if (resolved == null && mPersistentCache != null)
    {
      final ContactLocationCache.Entry cached = mPersistentCache.get(key);
      if (cached != null)
      {
        resolved = new ResolvedAddress(cached.lat, cached.lon, cached.estimated);
        mCache.put(key, resolved);
      }
    }
    return resolved;
  }

  @NonNull
  private static String normalizeKey(@NonNull ContactAddress contactAddress)
  {
    return contactAddress.getAddressKey();
  }

  private void onViewportChanged(int scale, @NonNull String region)
  {
    cancelRequests();
    mRequests.clear();
    mQueue.clear();
    mFailed.clear();
    mViewportScale = scale;
    mViewportRegionTokens = ContactAddressNormalizer.matchTokens(region);
    mMainHandler.removeCallbacks(mResolveViewport);
    if (scale >= 16)
      mMainHandler.postDelayed(mResolveViewport, 300);
  }

  private void resolveViewport()
  {
    if (mViewportScale < 16 || mContext == null || !isEnabled(mContext))
      return;
    for (PendingAddress address : mAddresses.values())
    {
      if (mCache.containsKey(address.key) || mFailed.contains(address.key) || mRequests.values().contains(address)
          || mQueue.contains(address))
        continue;
      // Searches are bounded by the viewport; skip contacts in a different downloaded map region as well.
      if (!ContactAddressNormalizer.matchesMapRegion(address.contextTokens, mViewportRegionTokens))
        continue;
      mQueue.add(address);
    }
    resolveNext();
  }

  private void cancelRequests()
  {
    for (long requestId : mRequests.keySet())
      SearchEngine.INSTANCE.cancelContactAddressResolution(requestId);
  }

  @MainThread
  private void scheduleMarksUpdate()
  {
    mMainHandler.removeCallbacks(mUpdateMarks);
    mMainHandler.postDelayed(mUpdateMarks, MARK_UPDATE_DELAY_MS);
  }

  @MainThread
  private void updateMarks()
  {
    final Map<String, ContactMark> marksByPosition = new LinkedHashMap<>();
    for (String key : mVisibleKeys)
    {
      final ResolvedAddress coordinate = mCache.get(key);
      if (coordinate == null)
        continue;
      final String position = Math.round(coordinate.lat * 100000.0) + ":" + Math.round(coordinate.lon * 100000.0);
      final ContactMark mark = marksByPosition.computeIfAbsent(position, ignored -> new ContactMark(coordinate));
      final Set<String> names = mNames.get(key);
      if (names != null)
        mark.names.addAll(names);
    }

    final double[] latitudes = new double[marksByPosition.size()];
    final double[] longitudes = new double[marksByPosition.size()];
    final String[] names = new String[marksByPosition.size()];
    final boolean[] estimated = new boolean[marksByPosition.size()];
    int index = 0;
    for (ContactMark mark : marksByPosition.values())
    {
      latitudes[index] = mark.coordinate.lat;
      longitudes[index] = mark.coordinate.lon;
      names[index] = String.join(", ", mark.names);
      estimated[index] = mark.coordinate.estimated;
      ++index;
    }
    if (Arrays.equals(latitudes, mMarkLatitudes) && Arrays.equals(longitudes, mMarkLongitudes)
        && Arrays.equals(names, mMarkNames) && Arrays.equals(estimated, mMarkEstimated))
      return;
    mMarkLatitudes = latitudes;
    mMarkLongitudes = longitudes;
    mMarkNames = names;
    mMarkEstimated = estimated;
    Framework.nativeSetContactMarks(latitudes, longitudes, names, estimated);
  }
}
