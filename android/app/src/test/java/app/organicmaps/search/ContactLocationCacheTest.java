package app.organicmaps.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import org.junit.Test;

public class ContactLocationCacheTest
{
  @Test
  public void hashesAddressesDeterministicallyWithoutStoringAddressText()
  {
    final String address = "11378 158A Street Surrey";
    final String hash = ContactLocationCache.hash(address);

    assertEquals(hash, ContactLocationCache.hash(address));
    assertEquals(64, hash.length());
    assertNotEquals(address, hash);
  }
}
