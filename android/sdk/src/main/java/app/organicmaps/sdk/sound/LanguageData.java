package app.organicmaps.sdk.sound;

import android.speech.tts.TextToSpeech;
import java.util.Locale;

/**
 * {@code LanguageData} describes single voice language managed by {@link TtsPlayer}.
 * Supported languages are listed in {@code strings-tts.xml} file, for details see comments there.
 */
public class LanguageData
{
  static class NotAvailableException extends Exception
  {
    NotAvailableException(Locale locale)
    {
      super("Locale \"" + locale + "\" is not supported by current TTS engine");
    }
  }

  public final Locale locale;
  public final String name;
  public final String internalCode;
  public final boolean downloaded;

  LanguageData(String line, String name, TextToSpeech tts) throws NotAvailableException, IllegalArgumentException
  {
    this.name = name;

    String[] parts = line.split(":");
    String code = (parts.length > 1 ? parts[1] : null);

    parts = parts[0].split("-");
    String language = parts[0];
    internalCode = (code == null ? language : code);

    String country = (parts.length > 1 ? parts[1] : "");
    locale = new Locale(language, country);

    // tts.isLanguageAvailable() may throw IllegalArgumentException if the TTS is corrupted internally.
    int status = tts.isLanguageAvailable(locale);
    if (status < TextToSpeech.LANG_MISSING_DATA)
      throw new NotAvailableException(locale);

    downloaded = (status >= TextToSpeech.LANG_AVAILABLE);
  }

  boolean matchesLocale(Locale locale)
  {
    String lang = locale.getLanguage();
    if (!lang.equals(this.locale.getLanguage()))
      return false;

    if ("zh".equals(lang))
    {
      // Chinese spoken languages: Cantonese for HK/MO/Guangdong,
      // Mandarin Traditional (zh-Hant) for TW,
      // Mandarin Simplified (zh-Hans) for everything else.
      String country = locale.getCountry();
      if ("yue-HK".equals(internalCode))
        return "HK".equals(country);
      if ("yue-MO".equals(internalCode))
        return "MO".equals(country);
      if ("yue".equals(internalCode))
        return false; // Generic Cantonese fallback; not auto-selected.
      if ("zh-Hant".equals(internalCode))
        return "TW".equals(country);
      return "zh-Hans".equals(internalCode);
    }

    return true;
  }

  boolean matchesInternalCode(String internalCode)
  {
    return this.internalCode.equals(internalCode);
  }

  @Override
  public String toString()
  {
    return name + ": " + locale + ", internal: " + internalCode + (downloaded ? " - " : " - NOT ") + "downloaded";
  }
}
