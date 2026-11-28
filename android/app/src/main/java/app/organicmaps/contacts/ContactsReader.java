package app.organicmaps.contacts;

import android.content.ContentResolver;
import android.database.Cursor;
import android.os.CancellationSignal;
import android.provider.ContactsContract;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

final class ContactsReader
{
  private static final String[] PROJECTION = {ContactsContract.Data.CONTACT_ID,
                                              ContactsContract.Contacts.LOOKUP_KEY,
                                              ContactsContract.Contacts.DISPLAY_NAME_PRIMARY,
                                              ContactsContract.Data.IS_SUPER_PRIMARY,
                                              ContactsContract.Data.IS_PRIMARY,
                                              ContactsContract.CommonDataKinds.StructuredPostal.FORMATTED_ADDRESS,
                                              ContactsContract.CommonDataKinds.StructuredPostal.STREET,
                                              ContactsContract.CommonDataKinds.StructuredPostal.CITY,
                                              ContactsContract.CommonDataKinds.StructuredPostal.REGION,
                                              ContactsContract.CommonDataKinds.StructuredPostal.POSTCODE,
                                              ContactsContract.CommonDataKinds.StructuredPostal.COUNTRY};

  private ContactsReader() {}

  static List<ContactsImporter.Contact> read(ContentResolver resolver, CancellationSignal cancellation)
  {
    // Only postal rows are read. Primary addresses come first; row IDs make ties deterministic.
    String order = ContactsContract.Data.IS_SUPER_PRIMARY + " DESC," + ContactsContract.Data.IS_PRIMARY + " DESC,"
                 + ContactsContract.Data._ID;
    try (Cursor cursor = resolver.query(
             ContactsContract.Data.CONTENT_URI, PROJECTION, ContactsContract.Data.MIMETYPE + "=?",
             new String[] {ContactsContract.CommonDataKinds.StructuredPostal.CONTENT_ITEM_TYPE}, order, cancellation))
    {
      if (cursor == null)
        throw new IllegalStateException("Contacts provider unavailable");
      return read(cursor, cancellation);
    }
  }

  static List<ContactsImporter.Contact> read(Cursor cursor, CancellationSignal cancellation)
  {
    Map<Long, ContactsImporter.Contact> contacts = new LinkedHashMap<>();
    int[] columns = new int[PROJECTION.length];
    for (int i = 0; i < columns.length; ++i)
      columns[i] = cursor.getColumnIndexOrThrow(PROJECTION[i]);
    while (cursor.moveToNext())
    {
      cancellation.throwIfCanceled();
      long id = cursor.getLong(columns[0]);
      String lookup = value(cursor, columns[1]);
      String name = value(cursor, columns[2]);
      if (id <= 0 || name.isEmpty())
        continue;
      ContactsImporter.Contact contact = contacts.computeIfAbsent(
          id,
          ignored -> new ContactsImporter.Contact(hashKey(lookup.isEmpty() ? "id:" + id : "lookup:" + lookup), name));
      String street = value(cursor, columns[6]);
      String city = value(cursor, columns[7]);
      String region = value(cursor, columns[8]);
      String country = value(cursor, columns[10]);
      String address = ContactAddressNormalizer.format(value(cursor, columns[5]), street, city, region,
                                                       value(cursor, columns[9]), country);
      if (!address.isEmpty() && address.length() <= 4096)
        contact.addAddress(new ContactAddress(address, street, city, region, country));
    }
    return new ArrayList<>(contacts.values());
  }

  private static String value(Cursor cursor, int column)
  {
    String value = cursor.getString(column);
    return value == null ? "" : value.trim();
  }

  static String hashKey(String key)
  {
    return ContactSourceKey.hash(key);
  }
}
