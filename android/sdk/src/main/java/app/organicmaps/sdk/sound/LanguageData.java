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
      // Auto-pick one Chinese variant per region: Cantonese for HK/MO,
      // Traditional script for TW, Simplified script everywhere else.
      // Generic "yue" is intentionally never auto-selected (user picks it manually).
      String expectedCode = switch (locale.getCountry())
      {
        case "HK" -> "yue-HK";
        case "MO" -> "yue-MO";
        case "TW" -> "zh-Hant";
        default -> "zh-Hans";
      };
      return expectedCode.equals(internalCode);
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
