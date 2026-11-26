package app.organicmaps.contacts;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.*;

import android.database.Cursor;
import android.os.CancellationSignal;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;

public class ContactsReaderTest
{
  @Test
  public void groupsPostalRowsAndRemovesDuplicatesWithoutReadingOtherContactData()
  {
    Cursor cursor = mock(Cursor.class);
    CancellationSignal cancellation = mock(CancellationSignal.class);
    AtomicInteger column = new AtomicInteger();
    when(cursor.getColumnIndexOrThrow(anyString())).thenAnswer(call -> column.getAndIncrement());
    when(cursor.moveToNext()).thenReturn(true, true, false);
    when(cursor.getLong(0)).thenReturn(17L);
    when(cursor.getString(1)).thenReturn("provider-key");
    when(cursor.getString(2)).thenReturn("Person");
    when(cursor.getString(5)).thenReturn("123 Main Street, Canada");
    when(cursor.getString(6)).thenReturn("123 Main Street");
    when(cursor.getString(10)).thenReturn("Canada");
    var contacts = ContactsReader.read(cursor, cancellation);
    assertEquals(1, contacts.size());
    assertEquals(1, contacts.get(0).addresses.size());
    assertEquals(ContactsReader.hashKey("lookup:provider-key"), contacts.get(0).key);
    verify(cancellation, times(2)).throwIfCanceled();
    verify(cursor, times(11)).getColumnIndexOrThrow(anyString());
  }

  @Test
  public void missingRequiredColumnsDoNotSilentlyLookLikeAnEmptyAddressBook()
  {
    Cursor cursor = mock(Cursor.class);
    when(cursor.getColumnIndexOrThrow(anyString())).thenThrow(new IllegalArgumentException());
    assertThrows(IllegalArgumentException.class, () -> ContactsReader.read(cursor, mock(CancellationSignal.class)));
  }

  @Test
  public void skipsRowsWithoutAContactName()
  {
    Cursor cursor = mock(Cursor.class);
    AtomicInteger column = new AtomicInteger();
    when(cursor.getColumnIndexOrThrow(anyString())).thenAnswer(call -> column.getAndIncrement());
    when(cursor.moveToNext()).thenReturn(true, false);
    when(cursor.getLong(0)).thenReturn(17L);
    assertTrue(ContactsReader.read(cursor, mock(CancellationSignal.class)).isEmpty());
  }
}
