package app.organicmaps.sdk.downloader;

import static org.junit.Assert.assertEquals;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import org.junit.Test;

public class CountryItemTest
{
  @Test
  public void comparatorUsesPolishAlphabetWithinEachCategory()
  {
    List<CountryItem> countries = new ArrayList<>();
    countries.add(item("lodz", "województwo łódzkie", CountryItem.CATEGORY_AVAILABLE));
    countries.add(item("masovia", "województwo mazowieckie", CountryItem.CATEGORY_AVAILABLE));
    countries.add(item("lublin", "województwo lubelskie", CountryItem.CATEGORY_AVAILABLE));
    countries.add(item("downloaded", "województwo śląskie", CountryItem.CATEGORY_DOWNLOADED));

    countries.sort(CountryItem.comparator(Locale.forLanguageTag("pl-PL")));

    assertEquals(List.of("downloaded", "lublin", "lodz", "masovia"), countries.stream().map(item -> item.id).toList());
  }

  private static CountryItem item(String id, String name, int category)
  {
    CountryItem item = new CountryItem(id);
    item.name = name;
    item.category = category;
    return item;
  }
}
