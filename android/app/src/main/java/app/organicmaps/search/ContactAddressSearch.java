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
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;

final class ContactAddressSearch
{
  interface Callback
  {
    void onResults(@NonNull String query, @NonNull List<ContactAddress> results);
  }

  private static final int MAX_RESULTS = 20;
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
  private volatile List<ContactAddress> mCachedAddresses;

  @NonNull
  static synchronized ContactAddressSearch getInstance(@NonNull Context context)
  {
    if (sInstance == null)
      sInstance = new ContactAddressSearch(context);
    return sInstance;
  }

  private ContactAddressSearch(@NonNull Context context)
  {
    mContext = context.getApplicationContext();
    mResolver = mContext.getContentResolver();
    mContactsObserver = new ContentObserver(mMainHandler) {
      @Override
      public void onChange(boolean selfChange)
      {
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

  static boolean hasPermission(@NonNull Context context)
  {
    final int permission = ContextCompat.checkSelfPermission(context, Manifest.permission.READ_CONTACTS);
    return permission == PackageManager.PERMISSION_GRANTED;
  }

  void search(@NonNull String query, @NonNull Callback callback)
  {
    final String normalizedQuery = query.trim().toLowerCase(Locale.ROOT);
    if (normalizedQuery.isEmpty())
    {
      callback.onResults(query, Collections.emptyList());
      return;
    }

    ThreadPool.getWorker().execute(() -> {
      final List<ContactAddress> matches = findMatches(normalizedQuery);
      mMainHandler.post(() -> {
        callback.onResults(query, matches);
      });
    });
  }

  @NonNull
  private List<ContactAddress> findMatches(@NonNull String normalizedQuery)
  {
    return findMatches(getAddresses(), normalizedQuery);
  }

  @NonNull
  static List<ContactAddress> findMatches(@NonNull List<ContactAddress> addresses, @NonNull String normalizedQuery)
  {
    final List<ContactAddress> matches = new ArrayList<>();
    for (ContactAddress address : addresses)
    {
      if (address.normalizedName.contains(normalizedQuery))
        matches.add(address);
    }
    matches.sort(
        Comparator
            .comparingInt(
                (ContactAddress address) -> address.normalizedName.startsWith(normalizedQuery) ? 0 : 1)
            .thenComparing(address -> address.name, String.CASE_INSENSITIVE_ORDER)
            .thenComparing(address -> address.label, String.CASE_INSENSITIVE_ORDER));
    final Set<String> names = new HashSet<>();
    final List<ContactAddress> limitedMatches = new ArrayList<>();
    for (ContactAddress match : matches)
    {
      if (!names.contains(match.name) && names.size() == 2)
        continue;
      names.add(match.name);
      limitedMatches.add(match);
      if (limitedMatches.size() == MAX_RESULTS)
        break;
    }
    return limitedMatches;
  }

  @NonNull
  private synchronized List<ContactAddress> getAddresses()
  {
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

      while (cursor.moveToNext())
      {
        final String name = getString(cursor, ContactsContract.Contacts.DISPLAY_NAME_PRIMARY);
        if (name.isEmpty())
          continue;

        final String street = getString(cursor, ContactsContract.CommonDataKinds.StructuredPostal.STREET);
        final String locality = getString(cursor, ContactsContract.CommonDataKinds.StructuredPostal.CITY);
        final String address = ContactAddressNormalizer.format(
            getString(cursor, ContactsContract.CommonDataKinds.StructuredPostal.FORMATTED_ADDRESS), street,
            getString(cursor, ContactsContract.CommonDataKinds.StructuredPostal.POBOX),
            getString(cursor, ContactsContract.CommonDataKinds.StructuredPostal.NEIGHBORHOOD), locality,
            getString(cursor, ContactsContract.CommonDataKinds.StructuredPostal.REGION),
            getString(cursor, ContactsContract.CommonDataKinds.StructuredPostal.POSTCODE),
            getString(cursor, ContactsContract.CommonDataKinds.StructuredPostal.COUNTRY));
        if (address.isEmpty())
          continue;

        final int type =
            cursor.getInt(cursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.StructuredPostal.TYPE));
        final String customLabel = getString(cursor, ContactsContract.CommonDataKinds.StructuredPostal.LABEL);
        final String label =
            ContactsContract.CommonDataKinds.StructuredPostal.getTypeLabel(mContext.getResources(), type, customLabel)
                .toString();
        final String key = name + '\u0000' + label + '\u0000' + address;
        if (deduplicationKeys.add(key))
          addresses.add(new ContactAddress(name, label, address, street, locality));
      }
    }
    catch (SecurityException ignored)
    {
      return Collections.emptyList();
    }

    final List<ContactAddress> loadedAddresses = Collections.unmodifiableList(addresses);
    if (cacheGeneration == mCacheGeneration.get())
      mCachedAddresses = loadedAddresses;
    return loadedAddresses;
  }

  void loadAll(@NonNull Callback callback)
  {
    ThreadPool.getWorker().execute(() -> {
      final List<ContactAddress> addresses = getAddresses();
      mMainHandler.post(() -> {
        callback.onResults("", addresses);
      });
    });
  }

  @NonNull
  private static String getString(@NonNull Cursor cursor, @NonNull String column)
  {
    final String value = cursor.getString(cursor.getColumnIndexOrThrow(column));
    return value == null ? "" : value.trim();
  }

}
