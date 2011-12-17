package com.mapswithme.maps;

import java.util.Locale;

public class UnitLocale {
    public static UnitLocale Imperial = new UnitLocale();
    public static UnitLocale Metric = new UnitLocale();

    public static UnitLocale getCurrent()
    {
        String countryCode = Locale.getDefault().getCountry();
        if ("US".equals(countryCode)) return Imperial; // USA
        if ("LR".equals(countryCode)) return Imperial; // liberia
        if ("MM".equals(countryCode)) return Imperial; // burma
        return Metric;
    }
}
