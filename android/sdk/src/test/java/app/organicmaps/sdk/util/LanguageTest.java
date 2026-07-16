package app.organicmaps.sdk.util;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

// Only toModernLanguageTag() is covered: it is the one part of Language free of Android APIs.
// These tests run on the JVM, whose java.util.Locale differs from Android's -- notably it no longer
// stores the deprecated codes -- so they deliberately pass string literals and never build a Locale.
public class LanguageTest
{
  // Android's Locale rewrites "id"/"he"/"yi" to the deprecated "in"/"iw"/"ji" and keeps them
  // internally. toLanguageTag() undoes that only since API 24, so getDefaultLocale() must undo it
  // itself or the core, which only knows the modern codes, would not recognise the language.
  @Test
  public void toModernLanguageTag_replacesDeprecatedCodes()
  {
    assertEquals("id", Language.toModernLanguageTag("in"));
    assertEquals("id-ID", Language.toModernLanguageTag("in-ID"));
    assertEquals("he-IL", Language.toModernLanguageTag("iw-IL"));
    assertEquals("yi", Language.toModernLanguageTag("ji"));
    assertEquals("yi-US", Language.toModernLanguageTag("ji-US"));
  }

  // A no-op on API 24+, where toLanguageTag() has already replaced them.
  @Test
  public void toModernLanguageTag_keepsModernCodes()
  {
    assertEquals("id-ID", Language.toModernLanguageTag("id-ID"));
    assertEquals("he-IL", Language.toModernLanguageTag("he-IL"));
    assertEquals("en-UA", Language.toModernLanguageTag("en-UA"));
    assertEquals("zh-Hans-CN", Language.toModernLanguageTag("zh-Hans-CN"));
    assertEquals("pt-BR", Language.toModernLanguageTag("pt-BR"));
    assertEquals("", Language.toModernLanguageTag(""));
  }

  // The primary subtag is compared as a whole: three-letter codes that merely start with a
  // deprecated one keep their own language.
  @Test
  public void toModernLanguageTag_matchesWholePrimarySubtag()
  {
    assertEquals("inh", Language.toModernLanguageTag("inh"));
    assertEquals("inh-RU", Language.toModernLanguageTag("inh-RU"));
    assertEquals("ind-ID", Language.toModernLanguageTag("ind-ID"));
  }

  // An IME subtype that declares only the older android:imeSubtypeLocale reports a POSIX-style
  // locale instead of a tag. Hyphens make it one, so that the deprecated code is replaced here too
  // and the core sees the region: "en-US" and "pt-BR" are categories.txt locales, "en_US" is not.
  @Test
  public void toModernLanguageTag_convertsPosixLocales()
  {
    assertEquals("en-US", Language.toModernLanguageTag("en_US"));
    assertEquals("pt-BR", Language.toModernLanguageTag("pt_BR"));
    assertEquals("zh-TW", Language.toModernLanguageTag("zh_TW"));
    assertEquals("id-ID", Language.toModernLanguageTag("in_ID"));
    assertEquals("he-IL", Language.toModernLanguageTag("iw_IL"));
    assertEquals("inh-RU", Language.toModernLanguageTag("inh_RU"));
  }
}
