package app.organicmaps.search;

import android.Manifest;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.ContentObserver;
import android.database.Cursor;
import android.os.Handler;
import android.os.Looper;
import android.provider.ContactsContract;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;

final class ContactAddressSearch
{
  interface Callback
  {
    void onResults(@NonNull String query, @NonNull List<ContactAddress> results);
  }

  private static final int MAX_RESULTS = 20;
  private static final Comparator<ContactAddress> ADDRESS_ORDER =
      Comparator.comparing((ContactAddress address) -> address.name, String.CASE_INSENSITIVE_ORDER)
          .thenComparing(address -> address.label, String.CASE_INSENSITIVE_ORDER);
  private static ContactAddressSearch sInstance;
  private static final String[] PROJECTION = {
      ContactsContract.Contacts.DISPLAY_NAME_PRIMARY,
      ContactsContract.CommonDataKinds.StructuredPostal.TYPE,
      ContactsContract.CommonDataKinds.StructuredPostal.LABEL,
      ContactsContract.CommonDataKinds.StructuredPostal.FORMATTED_ADDRESS,
      ContactsContract.CommonDataKinds.StructuredPostal.STREET,
      ContactsContract.CommonDataKinds.StructuredPostal.POBOX,
      ContactsContract.CommonDataKinds.StructuredPostal.NEIGHBORHOOD,
      ContactsContract.CommonDataKinds.StructuredPostal.CITY,
      ContactsContract.CommonDataKinds.StructuredPostal.REGION,
      ContactsContract.CommonDataKinds.StructuredPostal.POSTCODE,
      ContactsContract.CommonDataKinds.StructuredPostal.COUNTRY,
  };

  @NonNull
  private final Context mContext;
  @NonNull
  private final ContentResolver mResolver;
  @NonNull
  private final Handler mMainHandler = new Handler(Looper.getMainLooper());
  @NonNull
  private final ContentObserver mContactsObserver;
  @NonNull
  private final Set<Runnable> mContactsChangedListeners = new HashSet<>();
  @NonNull
  private final AtomicInteger mCacheGeneration = new AtomicInteger();
  private final AtomicInteger mSearchGeneration = new AtomicInteger();
  private volatile List<ContactAddress> mCachedAddresses;
  private volatile boolean mShutdown;
  private boolean mObserverRegistered;

  @NonNull
  static synchronized ContactAddressSearch getInstance(@NonNull Context context)
  {
    if (sInstance == null)
      sInstance = new ContactAddressSearch(context);
    return sInstance;
  }

  static synchronized void shutdown()
  {
    if (sInstance == null)
      return;
    sInstance.close();
    sInstance = null;
  }

  private ContactAddressSearch(@NonNull Context context)
  {
    mContext = context.getApplicationContext();
    mResolver = mContext.getContentResolver();
    mContactsObserver = new ContentObserver(mMainHandler) {
      @Override
      public void onChange(boolean selfChange)
      {
        if (mShutdown)
          return;
        mCacheGeneration.incrementAndGet();
        mCachedAddresses = null;
        ThreadPool.getWorker().execute(ContactAddressSearch.this::getAddresses);
        for (Runnable listener : List.copyOf(mContactsChangedListeners))
          listener.run();
      }
    };
    try
    {
      mResolver.registerContentObserver(ContactsContract.Data.CONTENT_URI, true, mContactsObserver);
      mObserverRegistered = true;
    }
    catch (SecurityException ignored)
    {}

    // Reading structured postal rows is the expensive part of contact matching. Queue it as soon
    // as contact search becomes active so the first typed query normally hits the memory cache.
    ThreadPool.getWorker().execute(this::getAddresses);
  }

  void addContactsChangedListener(@NonNull Runnable listener)
  {
    mContactsChangedListeners.add(listener);
  }

  private void close()
  {
    mShutdown = true;
    mCacheGeneration.incrementAndGet();
    mSearchGeneration.incrementAndGet();
    mCachedAddresses = null;
    mContactsChangedListeners.clear();
    if (mObserverRegistered)
    {
      mResolver.unregisterContentObserver(mContactsObserver);
      mObserverRegistered = false;
    }
  }

  static boolean hasPermission(@NonNull Context context)
  {
    final int permission = ContextCompat.checkSelfPermission(context, Manifest.permission.READ_CONTACTS);
    return permission == PackageManager.PERMISSION_GRANTED;
  }

  void search(@NonNull String query, @NonNull Callback callback)
  {
    final int generation = mSearchGeneration.incrementAndGet();
    if (mShutdown)
    {
      callback.onResults(query, Collections.emptyList());
      return;
    }
    if (query.trim().isEmpty() || containsDigits(query))
    {
      callback.onResults(query, Collections.emptyList());
      return;
    }

    ThreadPool.getWorker().execute(() -> {
      if (mShutdown || generation != mSearchGeneration.get())
        return;
      final List<ContactAddress> matches = findMatchesSorted(getAddresses(), query);
      mMainHandler.post(() -> {
        if (!mShutdown && generation == mSearchGeneration.get())
          callback.onResults(query, matches);
      });
    });
  }

  @NonNull
  static List<ContactAddress> findMatches(@NonNull List<ContactAddress> addresses, @NonNull String query)
  {
    final List<ContactAddress> sorted = new ArrayList<>(addresses);
    sorted.sort(ADDRESS_ORDER);
    return findMatchesSorted(sorted, query);
  }

  @NonNull
  private static List<ContactAddress> findMatchesSorted(@NonNull List<ContactAddress> addresses, @NonNull String query)
  {
    if (containsDigits(query))
      return Collections.emptyList();
    final String normalizedQuery = ContactAddress.normalizeName(query);
    if (normalizedQuery.isEmpty())
      return Collections.emptyList();
    final String[] queryWords = normalizedQuery.split(" ");
    final boolean shortQuery =
        normalizedQuery.codePointCount(0, normalizedQuery.length()) - (queryWords.length - 1) < 3;
    final Set<String> names = new HashSet<>();
    final List<ContactAddress> matches = new ArrayList<>(MAX_RESULTS);
    // Provider rows are sorted once. Prefer full-name prefixes without sorting all matches on each keystroke.
    for (boolean fullNamePrefix : new boolean[] {true, false})
      for (ContactAddress address : addresses)
      {
        if (address.normalizedName.startsWith(normalizedQuery) != fullNamePrefix
            || (!names.contains(address.name) && names.size() == 2))
          continue;
        if (shortQuery ? queryWords.length != 1 || !address.nameWords.contains(normalizedQuery)
                       : !matchesWords(address.nameWords, queryWords))
          continue;
        names.add(address.name);
        matches.add(address);
        if (matches.size() == MAX_RESULTS)
          return matches;
      }
    return matches;
  }

  private static boolean containsDigits(@NonNull String query)
  {
    // Name normalization removes numbers; do not turn an address into unrelated name prefixes.
    for (int offset = 0; offset < query.length();)
    {
      final int codePoint = query.codePointAt(offset);
      if (Character.isDigit(codePoint))
        return true;
      offset += Character.charCount(codePoint);
    }
    return false;
  }

  private static boolean matchesWords(@NonNull List<String> nameWords, @NonNull String[] queryWords)
  {
    for (String queryWord : queryWords)
    {
      boolean matched = false;
      for (String nameWord : nameWords)
        if (nameWord.startsWith(queryWord))
        {
          matched = true;
          break;
        }
      if (!matched)
        return false;
    }
    return true;
  }

  @NonNull
  private synchronized List<ContactAddress> getAddresses()
  {
    if (mShutdown)
      return Collections.emptyList();
    final List<ContactAddress> cached = mCachedAddresses;
    if (cached != null)
      return cached;

    final int cacheGeneration = mCacheGeneration.get();
    final List<ContactAddress> addresses = new ArrayList<>();
    final Set<String> deduplicationKeys = new HashSet<>();
    try (Cursor cursor = mResolver.query(ContactsContract.CommonDataKinds.StructuredPostal.CONTENT_URI, PROJECTION,
                                         null, null, null))
    {
      if (cursor == null)
        return Collections.emptyList();

      final int nameColumn = cursor.getColumnIndexOrThrow(ContactsContract.Contacts.DISPLAY_NAME_PRIMARY);
      final int typeColumn = cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.TYPE);
      final int labelColumn = cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.LABEL);
      final int formattedAddressColumn =
          cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.FORMATTED_ADDRESS);
      final int streetColumn = cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.STREET);
      final int poBoxColumn = cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.POBOX);
      final int neighborhoodColumn =
          cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.NEIGHBORHOOD);
      final int cityColumn = cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.CITY);
      final int regionColumn = cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.REGION);
      final int postcodeColumn =
          cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.POSTCODE);
      final int countryColumn = cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.COUNTRY);

      while (cursor.moveToNext())
      {
        if (mShutdown)
          return Collections.emptyList();
        final String name = getString(cursor, nameColumn);
        if (name.isEmpty())
          continue;

        final String street = getString(cursor, streetColumn);
        final String locality = getString(cursor, cityColumn);
        final String address = ContactAddressNormalizer.format(
            getString(cursor, formattedAddressColumn), street, getString(cursor, poBoxColumn),
            getString(cursor, neighborhoodColumn), locality, getString(cursor, regionColumn),
            getString(cursor, postcodeColumn), getString(cursor, countryColumn));
        if (address.isEmpty())
          continue;

        final int type = cursor.getInt(typeColumn);
        final String customLabel = getString(cursor, labelColumn);
        final String label =
            ContactsContract.CommonDataKinds.StructuredPostal.getTypeLabel(mContext.getResources(), type, customLabel)
                .toString();
        final String key = name + '\u0000' + label + '\u0000' + address;
        if (deduplicationKeys.add(key))
          addresses.add(new ContactAddress(name, label, address, street, locality, getString(cursor, regionColumn),
                                           getString(cursor, countryColumn)));
      }
    }
    catch (SecurityException ignored)
    {
      return Collections.emptyList();
    }

    addresses.sort(ADDRESS_ORDER);
    final List<ContactAddress> loadedAddresses = Collections.unmodifiableList(addresses);
    if (!mShutdown && cacheGeneration == mCacheGeneration.get())
      mCachedAddresses = loadedAddresses;
    return loadedAddresses;
  }

  void loadAll(@NonNull Callback callback)
  {
    ThreadPool.getWorker().execute(() -> {
      final List<ContactAddress> addresses = getAddresses();
      mMainHandler.post(() -> {
        if (!mShutdown)
          callback.onResults("", addresses);
      });
    });
  }

  @NonNull
  private static String getString(@NonNull Cursor cursor, int column)
  {
    final String value = cursor.getString(column);
    return value == null ? "" : value.trim();
  }
}
