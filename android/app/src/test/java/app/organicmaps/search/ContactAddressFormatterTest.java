package app.organicmaps.search;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class ContactAddressFormatterTest
{
  @Test
  public void normalizesMultilineFormattedAddress()
  {
    assertEquals("123 Main Street, Vancouver BC, Canada",
                 ContactAddressFormatter.format("123 Main Street\nVancouver BC\r\nCanada"));
  }

  @Test
  public void buildsAddressFromNonEmptyParts()
  {
    assertEquals(
        "123 Main Street, Vancouver, BC, V1V 1V1, Canada",
        ContactAddressFormatter.format(null, "123 Main Street", null, "", "Vancouver", "BC", "V1V 1V1", "Canada"));
  }

  @Test
  public void removesDuplicateParts()
  {
    assertEquals("Vancouver, BC", ContactAddressFormatter.format(null, "Vancouver", "Vancouver", "BC"));
  }
}
