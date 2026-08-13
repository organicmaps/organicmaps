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
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.Language;
import java.util.ArrayDeque;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

public enum ContactMapManager implements SearchEngine.ContactAddressListener
{
  INSTANCE;

  private static final int MAX_CONCURRENT_REQUESTS = 2;
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
    final List<ContactAddress.SearchQuery> queries;
    final long generation;
    int queryIndex;

    PendingAddress(@NonNull String key, @NonNull List<ContactAddress.SearchQuery> queries, long generation)
    {
      this.key = key;
      this.queries = queries;
      this.generation = generation;
    }

    @NonNull
    String currentQuery()
    {
      return queries.get(queryIndex).query;
    }

    boolean advance()
    {
      return ++queryIndex < queries.size();
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
  private long mNextRequestId;
  private boolean mPaused;

  ContactMapManager()
  {
    SearchEngine.INSTANCE.addContactAddressListener(this);
  }

  @MainThread
  public void refresh(@NonNull Context context)
  {
    cancelRequests();
    mContext = context.getApplicationContext();
    ++mGeneration;
    mQueue.clear();
    mRequests.clear();
    mVisibleKeys.clear();
    mNames.clear();
    mFailed.clear();
    mCache.clear();

    if (!isEnabled(mContext))
    {
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
    return Config.isContactSearchEnabled() &&
           ContextCompat.checkSelfPermission(context, Manifest.permission.READ_CONTACTS) ==
               PackageManager.PERMISSION_GRANTED;
  }

  @MainThread
  private void onAddressesLoaded(long generation, @NonNull List<ContactAddress> addresses)
  {
    if (generation != mGeneration || mContext == null || !isEnabled(mContext))
      return;

    final Map<String, PendingAddress> unique = new LinkedHashMap<>();
    for (ContactAddress address : addresses)
    {
      final List<ContactAddress.SearchQuery> searchQueries = address.getMapSearchQueries();
      if (!searchQueries.isEmpty())
      {
        final String key = normalizeKey(address);
        unique.putIfAbsent(key, new PendingAddress(key, searchQueries, generation));
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
      if (!mCache.containsKey(address.key) && !mFailed.contains(address.key))
        mQueue.add(address);
    }
    updateMarks();
    resolveNext();
  }

  @MainThread
  private void resolveNext()
  {
    if (mPaused)
      return;
    while (mRequests.size() < MAX_CONCURRENT_REQUESTS)
    {
      final PendingAddress address = mQueue.poll();
      if (address == null)
        return;
      if (address.generation != mGeneration)
        continue;
      final long requestId = ++mNextRequestId;
      mRequests.put(requestId, address);
      SearchEngine.INSTANCE.resolveContactAddress(address.currentQuery(), mLocale, requestId);
    }
  }

  @Override
  @MainThread
  public void onContactAddressResolved(long requestId, boolean found, double lat, double lon, boolean estimated)
  {
    final PendingAddress address = mRequests.remove(requestId);
    if (address == null || address.generation != mGeneration)
    {
      resolveNext();
      return;
    }

    if (mPaused)
    {
      mQueue.addFirst(address);
      return;
    }

    if (found)
    {
      final ResolvedAddress resolved = new ResolvedAddress(lat, lon, estimated);
      mCache.put(address.key, resolved);
      mPersistentCache.put(address.key, new ContactLocationCache.Entry(lat, lon, estimated));
      scheduleMarksUpdate();
    }
    else if (address.advance())
      mQueue.addFirst(address);
    else
      mFailed.add(address.key);
    resolveNext();
  }

  @MainThread
  public void pause()
  {
    mPaused = true;
    for (long requestId : mRequests.keySet())
      SearchEngine.INSTANCE.cancelContactAddressResolution(requestId);
  }

  @MainThread
  public void resume()
  {
    mPaused = false;
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
    if (mPersistentCache != null)
      mPersistentCache.put(key, new ContactLocationCache.Entry(lat, lon, estimated));
    mFailed.remove(key);
    mVisibleKeys.add(key);
    mNames.computeIfAbsent(key, ignored -> new LinkedHashSet<>()).add(contactAddress.name);
    scheduleMarksUpdate();
  }

  @Nullable
  ResolvedAddress getResolved(@NonNull ContactAddress contactAddress)
  {
    return mCache.get(normalizeKey(contactAddress));
  }

  @NonNull
  private static String normalizeKey(@NonNull ContactAddress contactAddress)
  {
    return ContactAddressNormalizer.normalizeAddressQuery(contactAddress.address).trim().toLowerCase(Locale.ROOT);
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
    Framework.nativeSetContactMarks(latitudes, longitudes, names, estimated);
  }
}
