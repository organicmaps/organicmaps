package app.organicmaps.sdk.util;

import android.content.Context;
import android.os.Build;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.util.Locale;

public class Language
{
  // The core expects a BCP 47 language tag, e.g. "en-UA", "zh-Hans-CN" or "pt-BR". Never report
  // Locale.toString() here: it is documented as a debugging aid and spells locales the POSIX way
  // ("pt_BR"), which does not match the core's hyphenated locale tables.
  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getDefaultLocale()
  {
    final Locale locale = Locale.getDefault();
    // toLanguageTag() spells a language-less locale as "und", so test the language itself.
    if (locale.getLanguage().isEmpty())
      return Locale.US.toLanguageTag();

    return toLanguageTag(locale);
  }

  // After some testing on Galaxy S4, looks like this method doesn't work on all devices:
  // sometime it always returns the same value as getDefaultLocale()
  @NonNull
  public static String getKeyboardLocale(@NonNull Context context)
  {
    final InputMethodManager imm = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
    if (imm == null)
      return getDefaultLocale();

    final InputMethodSubtype ims = imm.getCurrentInputMethodSubtype();
    if (ims == null)
      return getDefaultLocale();

    final String locale = getSubtypeLanguageTag(ims);
    if (locale.isEmpty())
      return getDefaultLocale();

    return toModernLanguageTag(locale);
  }

  // Drops the Unicode extensions that Android 14+ uses to store regional preferences, so that
  // "en-UA-u-fw-mon-ms-metric-mu-celsius" (first day of week, measurement system, temperature unit)
  // becomes plain "en-UA". They say nothing about the language.
  @NonNull
  private static String toLanguageTag(@NonNull Locale locale)
  {
    // stripExtensions() needs API 26 and is not backported by core library desugaring. Regional
    // preferences did not exist before Android 14, so there is nothing to strip on older devices.
    final Locale stripped = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ? locale.stripExtensions() : locale;
    return toModernLanguageTag(stripped.toLanguageTag());
  }

  // Spells any locale Android reports the way the core expects it.
  // Locale stores the deprecated ISO 639 codes internally, rewriting "id" as "in", "he" as "iw"
  // and "yi" as "ji". toLanguageTag() is documented to emit the modern codes that the core knows,
  // but only does so since API 24: Lollipop and Marshmallow return the stored code verbatim, and an
  // IME is free to declare android:languageTag="in" by hand, so undo the rewrite here.
  // The primary subtag is compared as a whole, otherwise "inh" (Ingush) would become Indonesian.
  // Package-private for LanguageTest: it is the only part of this class free of Android APIs.
  @NonNull
  static String toModernLanguageTag(@NonNull String locale)
  {
    // A legacy IME subtype spells its locale the POSIX way ("in_ID"), see getSubtypeLanguageTag().
    final String tag = locale.replace('_', '-');
    final int end = tag.indexOf('-');
    final String primary = end < 0 ? tag : tag.substring(0, end);
    final String rest = end < 0 ? "" : tag.substring(end);

    return switch (primary)
    {
      case "in" -> "id" + rest;
      case "iw" -> "he" + rest;
      case "ji" -> "yi" + rest;
      default -> tag;
    };
  }

  @NonNull
  private static String getSubtypeLanguageTag(@NonNull InputMethodSubtype ims)
  {
    // InputMethodSubtype.getLocale() is deprecated and spells the locale the POSIX way ("en_US").
    // Its BCP 47 replacement needs API 24; below that toModernLanguageTag() turns it into a tag.
    final String tag = Build.VERSION.SDK_INT >= Build.VERSION_CODES.N ? ims.getLanguageTag() : ims.getLocale();
    return tag == null ? "" : tag.trim();
  }
}
