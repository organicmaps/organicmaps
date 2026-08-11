package app.organicmaps.search;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
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

  private static final class Coordinate
  {
    final double lat;
    final double lon;

    Coordinate(double lat, double lon)
    {
      this.lat = lat;
      this.lon = lon;
    }
  }

  private static final class ContactMark
  {
    @NonNull
    final Coordinate coordinate;
    @NonNull
    final Set<String> names = new LinkedHashSet<>();

    ContactMark(@NonNull Coordinate coordinate)
    {
      this.coordinate = coordinate;
    }
  }

  private static final class PendingAddress
  {
    @NonNull
    final String key;
    @NonNull
    final String query;
    final long generation;

    PendingAddress(@NonNull String key, @NonNull String query, long generation)
    {
      this.key = key;
      this.query = query;
      this.generation = generation;
    }
  }

  private final Map<String, Coordinate> mCache = new HashMap<>();
  private final Map<String, Set<String>> mNames = new HashMap<>();
  private final Set<String> mFailed = new HashSet<>();
  private final ArrayDeque<PendingAddress> mQueue = new ArrayDeque<>();
  private final Map<Long, PendingAddress> mRequests = new HashMap<>();
  private final Set<String> mVisibleKeys = new HashSet<>();
  private ContactAddressSearch mContactSearch;
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

    if (!isEnabled(mContext))
    {
      Framework.nativeSetContactMarks(new double[] {}, new double[] {}, new String[] {});
      return;
    }

    mLocale = Language.getKeyboardLocale(mContext);
    if (mContactSearch == null)
      mContactSearch = new ContactAddressSearch(mContext, () -> refresh(mContext));
    final long generation = mGeneration;
    mContactSearch.loadAll((ignored, addresses) -> onAddressesLoaded(generation, addresses));
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
      final ContactAddress.SearchQuery searchQuery = address.getMapSearchQuery();
      final String key = searchQuery.query.trim().toLowerCase(Locale.ROOT);
      if (!key.isEmpty())
      {
        unique.putIfAbsent(key, new PendingAddress(key, searchQuery.query, generation));
        mNames.computeIfAbsent(key, ignored -> new LinkedHashSet<>()).add(address.name);
      }
    }

    mVisibleKeys.addAll(unique.keySet());
    for (PendingAddress address : unique.values())
    {
      if (!mCache.containsKey(address.key) && !mFailed.contains(address.key))
        mQueue.add(address);
    }
    updateMarks();
    resolveNext();
  }

  @MainThread
  private void resolveNext()
  {
    if (mPaused || !mRequests.isEmpty())
      return;
    final PendingAddress address = mQueue.poll();
    if (address == null || address.generation != mGeneration)
      return;

    final long requestId = ++mNextRequestId;
    mRequests.put(requestId, address);
    SearchEngine.INSTANCE.resolveContactAddress(address.query, mLocale, requestId);
  }

  @Override
  @MainThread
  public void onContactAddressResolved(long requestId, boolean found, double lat, double lon)
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
      mCache.put(address.key, new Coordinate(lat, lon));
      updateMarks();
    }
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
  void recordResolved(@NonNull ContactAddress contactAddress, double lat, double lon)
  {
    if (mContext == null || !isEnabled(mContext))
      return;
    final String key = contactAddress.getMapSearchQuery().query.trim().toLowerCase(Locale.ROOT);
    mCache.put(key, new Coordinate(lat, lon));
    mFailed.remove(key);
    mVisibleKeys.add(key);
    mNames.computeIfAbsent(key, ignored -> new LinkedHashSet<>()).add(contactAddress.name);
    updateMarks();
  }

  private void cancelRequests()
  {
    for (long requestId : mRequests.keySet())
      SearchEngine.INSTANCE.cancelContactAddressResolution(requestId);
  }

  @MainThread
  private void updateMarks()
  {
    final Map<String, ContactMark> marksByPosition = new LinkedHashMap<>();
    for (String key : mVisibleKeys)
    {
      final Coordinate coordinate = mCache.get(key);
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
    int index = 0;
    for (ContactMark mark : marksByPosition.values())
    {
      latitudes[index] = mark.coordinate.lat;
      longitudes[index] = mark.coordinate.lon;
      names[index] = String.join(", ", mark.names);
      ++index;
    }
    Framework.nativeSetContactMarks(latitudes, longitudes, names);
  }
}
