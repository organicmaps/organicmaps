package com.mapswithme.maps.sound;

import java.util.Locale;

public class LanguageData
{
  public final Locale locale;
  public final String name;
  public final String internalCode;

  private int mStatus;

  private LanguageData(String name, Locale locale, String internalCode)
  {
    this.locale = locale;
    this.name = name;
    this.internalCode = internalCode;
  }

  static LanguageData parse(String line, String name)
  {
    String[] parts = line.split(":");
    String internalCode = (parts.length > 1 ? parts[1] : null);

    parts = parts[0].split("-");
    String language = parts[0];
    String country = (parts.length > 1 ? parts[1] : "");
    Locale locale = new Locale(language, country.toUpperCase());

    return new LanguageData(name, locale, (internalCode == null ? language : internalCode));
  }

  void setStatus(int status)
  {
    mStatus = status;
  }

  public int getStatus()
  {
    return mStatus;
  }

  public boolean matchesLocale(Locale locale)
  {
    String lang = locale.getLanguage();
    if (!lang.equals(this.locale.getLanguage()))
      return false;

    if ("zh".equals(lang) && "zh-Hant".equals(internalCode))
    {
      // Chinese is a special case
      String country = locale.getCountry();
      return "TW".equals(country) ||
             "MO".equals(country) ||
             "HK".equals(country);
    }

    return true;
  }

  public boolean matchesInternalCode(String internalCode)
  {
    return this.internalCode.equals(internalCode);
  }

  @Override
  public String toString()
  {
    return "(" + mStatus + ") " + name + ": " + locale + ", internal: " + internalCode;
  }
}
