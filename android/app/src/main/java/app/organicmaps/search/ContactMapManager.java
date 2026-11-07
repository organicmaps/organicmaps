package app.organicmaps.search;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import app.organicmaps.BuildConfig;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.search.SearchEngine.AddressResolutionListener;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.Language;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public enum ContactMapManager implements AddressResolutionListener
{
  INSTANCE;

  private static final long MARK_UPDATE_DELAY_MS = 100;
  interface MarkerUpdater
  {
    void update(double[] latitudes, double[] longitudes, String[] names, String[] addresses);
  }
  private MarkerUpdater mMarkerUpdater = Framework::nativeSetContactMarks;
  interface DataVersionProvider
  {
    String get();
  }
  private DataVersionProvider mDataVersionProvider = SearchEngine::nativeGetAddressDataVersion;
  private String mDataVersion = "";
  private long mDataRevision = -1;

  static final class ResolvedAddress
  {
    final double lat;
    final double lon;

    ResolvedAddress(double lat, double lon)
    {
      this.lat = lat;
      this.lon = lon;
    }
  }

  private static final class ContactMark
  {
    @NonNull
    final ResolvedAddress coordinate;
    @NonNull
    final Set<String> names = new LinkedHashSet<>();
    final Set<String> addresses = new LinkedHashSet<>();

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
  private final Map<String, String> mPostalAddresses = new HashMap<>();
  private final ContactResolutionPolicy mResolutionPolicy = new ContactResolutionPolicy();
  private final ArrayDeque<PendingAddress> mQueue = new ArrayDeque<>();
  private record ActiveRequest(long id, PendingAddress address)
  {
  }
  @Nullable
  private ActiveRequest mActiveRequest;
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
  private String[] mMarkAddresses = {};

  @MainThread
  public void refresh(@NonNull Context context)
  {
    mMarkNames = null;
    cancelRequests();
    mMainHandler.removeCallbacks(mUpdateMarks);
    mContext = context.getApplicationContext();
    ++mGeneration;
    mQueue.clear();
    mAddresses.clear();
    mMainHandler.removeCallbacks(mResolveViewport);
    mVisibleKeys.clear();
    mNames.clear();
    mPostalAddresses.clear();
    mResolutionPolicy.clear();
    mCache.clear();
    mDataVersion = "";
    mDataRevision = -1;

    if (!isEnabled(mContext))
    {
      SearchEngine.INSTANCE.cancelAllAddressResolutions();
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
    // Address keys include the complete address. Reload names/addresses and retain only
    // surviving keys, rather than invalidating locations for unrelated provider updates.
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
        final String key = address.getAddressKey();
        unique.putIfAbsent(key, new PendingAddress(key, address.contextTokens, searchQueries, generation));
        mNames.computeIfAbsent(key, ignored -> new LinkedHashSet<>()).add(address.name);
        mPostalAddresses.put(key, address.address);
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
          mCache.put(address.key, new ResolvedAddress(cached.lat, cached.lon));
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
    while (mActiveRequest == null)
    {
      final PendingAddress address = mQueue.poll();
      if (address == null)
        return;
      if (address.generation != mGeneration || mCache.containsKey(address.key))
        continue;
      final long requestId = SearchEngine.INSTANCE.resolveAddress(
          address.queries.stream().map(query -> query.query).toArray(String[] ::new),
          address.queries.stream().map(query -> query.expectedStreet).toArray(String[] ::new), mLocale, true, this);
      mActiveRequest = new ActiveRequest(requestId, address);
    }
  }

  @Override
  @MainThread
  public void onAddressResolved(long requestId, boolean found, boolean complete, double lat, double lon)
  {
    if (mContext != null && !isEnabled(mContext))
    {
      refresh(mContext);
      return;
    }
    final ActiveRequest request = mActiveRequest;
    if (request == null || request.id != requestId || request.address.generation != mGeneration)
    {
      resolveNext();
      return;
    }
    mActiveRequest = null;
    final PendingAddress address = request.address;

    if (found)
    {
      final ResolvedAddress resolved = new ResolvedAddress(lat, lon);
      mCache.put(address.key, resolved);
      mPersistentCache.put(address.key, new ContactLocationCache.Entry(lat, lon));
      mResolutionPolicy.forget(address.key);
      scheduleMarksUpdate();
    }
    else
      mResolutionPolicy.recordMiss(address.key, SystemClock.elapsedRealtime());
    // Retain progress when the viewport changes instead of always starting with the first name.
    mAddresses.remove(address.key);
    mAddresses.put(address.key, address);
    resolveNext();
  }

  @MainThread
  void recordResolved(@NonNull ContactAddress contactAddress, double lat, double lon)
  {
    if (mContext == null || !isEnabled(mContext))
      return;
    final String key = contactAddress.getAddressKey();
    final ResolvedAddress resolved = new ResolvedAddress(lat, lon);
    mCache.put(key, resolved);
    mQueue.removeIf(address -> address.key.equals(key));
    if (mActiveRequest != null && mActiveRequest.address.key.equals(key))
      cancelRequests();
    if (mPersistentCache != null)
      mPersistentCache.put(key, new ContactLocationCache.Entry(lat, lon));
    mResolutionPolicy.forget(key);
    mVisibleKeys.add(key);
    mNames.computeIfAbsent(key, ignored -> new LinkedHashSet<>()).add(contactAddress.name);
    mPostalAddresses.put(key, contactAddress.address);
    scheduleMarksUpdate();
    resolveNext();
  }

  @Nullable
  ResolvedAddress getResolved(@NonNull ContactAddress contactAddress)
  {
    if (mContext == null || !isEnabled(mContext))
      return null;
    final String key = contactAddress.getAddressKey();
    ResolvedAddress resolved = mCache.get(key);
    if (resolved != null && mPersistentCache != null && mPersistentCache.get(key) == null)
    {
      mCache.remove(key);
      resolved = null;
      scheduleMarksUpdate();
    }
    if (resolved == null && mPersistentCache != null)
    {
      final ContactLocationCache.Entry cached = mPersistentCache.get(key);
      if (cached != null)
      {
        resolved = new ResolvedAddress(cached.lat, cached.lon);
        mCache.put(key, resolved);
      }
    }
    return resolved;
  }

  @MainThread
  void recordUnresolved(@NonNull ContactAddress contactAddress, boolean complete, @NonNull String locale)
  {
    if (mContext == null || !isEnabled(mContext))
      return;
    final String key = contactAddress.getAddressKey();
    final String context = resolutionContext(contactAddress.getSearchQueries(), locale);
    if (complete && !context.isEmpty() && mPersistentCache != null)
    {
      mPersistentCache.putUnresolved(key, context);
      mResolutionPolicy.forget(key);
    }
    else
      mResolutionPolicy.recordForegroundMiss(key, SystemClock.elapsedRealtime());
    mQueue.removeIf(address -> address.key.equals(key));
    if (mActiveRequest != null && mActiveRequest.address.key.equals(key))
      cancelRequests();
    resolveNext();
  }

  @MainThread
  boolean hasRecentResolutionMiss(@NonNull ContactAddress contactAddress, @NonNull String locale)
  {
    if (mContext == null || !isEnabled(mContext))
      return false;
    final String key = contactAddress.getAddressKey();
    if (mPersistentCache != null
        && mPersistentCache.isUnresolved(key, resolutionContext(contactAddress.getSearchQueries(), locale)))
      return true;
    return !mResolutionPolicy.shouldRetryForeground(key, SystemClock.elapsedRealtime());
  }

  private String resolutionContext(@NonNull List<ContactAddress.SearchQuery> queries, @NonNull String locale)
  {
    if (mDataVersion.isEmpty())
      return "";
    final StringBuilder context =
        new StringBuilder(mDataVersion).append('\n').append(BuildConfig.VERSION_CODE).append('\n').append(locale);
    for (ContactAddress.SearchQuery query : queries)
      context.append('\n').append(query.query).append('\n').append(query.expectedStreet);
    return ContactLocationCache.hash(context.toString());
  }

  private void onViewportChanged(int scale, @NonNull String mapRegions, long mapRevision, double left, double bottom,
                                 double right, double top)
  {
    if (mContext != null && !isEnabled(mContext))
    {
      refresh(mContext);
      return;
    }
    if (mapRevision != mDataRevision)
    {
      // Backgrounding and memory pressure also clear native caches without changing map data.
      String dataVersion = mDataVersionProvider.get();
      if (!mDataVersion.isEmpty() && !mDataVersion.equals(dataVersion) && mPersistentCache != null)
        mPersistentCache.clearUnresolved();
      mDataVersion = dataVersion;
      mDataRevision = mapRevision;
    }
    boolean areaChanged = mResolutionPolicy.updateArea(mapRegions, mapRevision, left, bottom, right, top);
    if (areaChanged && mPersistentCache != null)
      mCache.entrySet().removeIf(entry -> mPersistentCache.get(entry.getKey()) == null);
    boolean becameVisible = mViewportScale < 16 && scale >= 16;
    mViewportScale = scale;
    mViewportRegionTokens = ContactAddressNormalizer.matchTokens(mapRegions);
    if (areaChanged && scale >= 16)
    {
      // Re-submit cached marks when returning to an area without waiting for address searches.
      mMarkNames = null;
      updateMarks();
    }
    if (!areaChanged && !becameVisible && scale >= 16)
    {
      // Also allow expired misses to retry on a later small viewport movement.
      if (mActiveRequest == null && mQueue.isEmpty())
        resolveViewport();
      return;
    }
    if (mActiveRequest != null)
    {
      final PendingAddress address = mActiveRequest.address;
      mAddresses.remove(address.key);
      mAddresses.put(address.key, address);
    }
    cancelRequests();
    mQueue.clear();
    mMainHandler.removeCallbacks(mResolveViewport);
    if (scale >= 16)
      mMainHandler.postDelayed(mResolveViewport, 300);
  }

  private void resolveViewport()
  {
    if (mViewportScale < 16 || mContext == null || !isEnabled(mContext))
      return;
    final List<PendingAddress> candidates = new ArrayList<>(mAddresses.values());
    candidates.sort(
        (first, second)
            -> Integer.compare(ContactResolutionPolicy.regionScore(second.contextTokens, mViewportRegionTokens),
                               ContactResolutionPolicy.regionScore(first.contextTokens, mViewportRegionTokens)));
    for (PendingAddress address : candidates)
    {
      if (mCache.containsKey(address.key) || !mResolutionPolicy.shouldRetry(address.key, SystemClock.elapsedRealtime())
          || (mPersistentCache != null
              && mPersistentCache.isUnresolved(address.key, resolutionContext(address.queries, mLocale)))
          || (mActiveRequest != null && mActiveRequest.address == address) || mQueue.contains(address))
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
    final ActiveRequest request = mActiveRequest;
    mActiveRequest = null;
    if (request != null)
      SearchEngine.INSTANCE.cancelAddressResolution(request.id);
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
      final String address = mPostalAddresses.get(key);
      if (address != null)
        mark.addresses.add(address);
    }

    final double[] latitudes = new double[marksByPosition.size()];
    final double[] longitudes = new double[marksByPosition.size()];
    final String[] names = new String[marksByPosition.size()];
    final String[] addresses = new String[marksByPosition.size()];
    int index = 0;
    for (ContactMark mark : marksByPosition.values())
    {
      latitudes[index] = mark.coordinate.lat;
      longitudes[index] = mark.coordinate.lon;
      names[index] = String.join(", ", mark.names);
      addresses[index] = String.join("; ", mark.addresses);
      ++index;
    }
    if (Arrays.equals(latitudes, mMarkLatitudes) && Arrays.equals(longitudes, mMarkLongitudes)
        && Arrays.equals(names, mMarkNames) && Arrays.equals(addresses, mMarkAddresses))
      return;
    mMarkLatitudes = latitudes;
    mMarkLongitudes = longitudes;
    mMarkNames = names;
    mMarkAddresses = addresses;
    mMarkerUpdater.update(latitudes, longitudes, names, addresses);
  }
}
