package app.organicmaps.util;

import android.content.Context;
import android.util.Pair;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.text.NumberFormat;
import java.util.Formatter;
import java.util.Locale;

import app.organicmaps.Framework;
import app.organicmaps.util.log.Logger;

public class Speed
{
  private static String mUnitStringKmh = "km/h";
  private static String mUnitStringMiph = "mph";

  private static char mDecimalSeparator = Character.MIN_VALUE;

  public static double MpsToKmph(double mps) { return mps * 3.6; }
  public static double MpsToMiph(double mps) { return mps * 2.236936; }

  public static void setUnitStringKmh(String unitStringKmh) { mUnitStringKmh = unitStringKmh; }
  public static void setUnitStringMiph(String unitStringMiph) { mUnitStringMiph = unitStringMiph; }

  private final static DecimalFormat mDecimalFormatNoDecimal = new DecimalFormat("#");
  private final static DecimalFormat mDecimalFormatOneDecimal = new DecimalFormat("0.0");

  private final static Locale mLocale = Locale.getDefault();

  private final static StringBuilder mSb = new StringBuilder();
  private final static Formatter mFormatter = new Formatter(mSb, mLocale);

  private final static NumberFormat mNumberFormatNoDecimal = NumberFormat.getInstance(mLocale);
  private final static NumberFormat mNumberFormatOneDecimal = NumberFormat.getInstance(mLocale);

  public static Pair<String, String> formatMeasurements(double speedInMetersPerSecond, int units,
                                            Context context)
  {
    double speedValue;
    String unitsString;

    if (units == Framework.UNITS_IMPERIAL)
    {
      speedValue = MpsToMiph(speedInMetersPerSecond);
      unitsString = mUnitStringMiph;
    }
    else
    {
      speedValue = MpsToKmph(speedInMetersPerSecond);
      unitsString = mUnitStringKmh;
    }

    // Option 1: String.format()
    long start1 = System.nanoTime();
    String formatString = (speedValue < 10.0)? "%.1f" : "%.0f";
    String speedString = String.format(mLocale, formatString, speedValue);
    long elapsed1 = System.nanoTime() - start1;
    Logger.i("LOCALE_MEASURE", "1) " + speedString);

    // Option 2: DecimalFormat class
    long start2 = System.nanoTime();
    if (speedValue < 10.0)
      speedString = mDecimalFormatOneDecimal.format(speedValue);
    else
      speedString = mDecimalFormatNoDecimal.format(speedValue);
    long elapsed2 = System.nanoTime() - start2;
    Logger.i("LOCALE_MEASURE", "2) " + speedString);

    // Option 3: Long.toString() + StringBuffer.insert()
    long start3 = System.nanoTime();
    if (speedValue < 10.0)
    {
      speedString = Long.toString(Math.round(speedValue * 10.0));

      StringBuffer buffer = new StringBuffer(speedString);

      if (mDecimalSeparator == Character.MIN_VALUE)
        mDecimalSeparator = DecimalFormatSymbols.getInstance().getDecimalSeparator();

      // For low values (< 1.0), force to have 2 characters in string.
      if (buffer.length() < 2)
        buffer.insert(0, "0");

      buffer.insert(1, mDecimalSeparator);

      speedString = buffer.toString();
    }
    else
      speedString = Long.toString(Math.round(speedValue));
    long elapsed3 = System.nanoTime() - start3;
    Logger.i("LOCALE_MEASURE", "3) " + speedString);

    // Option 4: Formatter class
    mSb.setLength(0);
    long start4 = System.nanoTime();
    if (speedValue < 10.0)
      mFormatter.format("%.1f", speedValue);
    else
      mFormatter.format("%d", Math.round(speedValue));
    speedString = mSb.toString();
    long elapsed4 = System.nanoTime() - start4;
    Logger.i("LOCALE_MEASURE", "4) " + speedString);

    // Option 5: NumberFormat class
    mNumberFormatNoDecimal.setMaximumFractionDigits(0);
    mNumberFormatOneDecimal.setMaximumFractionDigits(1);
    long start5 = System.nanoTime();
    if (speedValue < 10.0)
      speedString = mNumberFormatOneDecimal.format(speedValue);
    else
      speedString = mNumberFormatNoDecimal.format(Math.round(speedValue));
    long elapsed5 = System.nanoTime() - start5;
    Logger.i("LOCALE_MEASURE", "5) " + speedString);

    String text = String.format(Locale.US,
            "Java calls: %5d / %5d / %5d / %5d / %5d",
            Math.round(0.001 * elapsed1),
            Math.round(0.001 * elapsed2),
            Math.round(0.001 * elapsed3),
            Math.round(0.001 * elapsed4),
            Math.round(0.001 * elapsed5));
    Logger.i("LOCALE_MEASURE", text);

    return new Pair<>(speedString, unitsString);
  }

  public static Pair<String, String> format(double speedInMetersPerSecond, int units,
                                                        Context context)
  {
    double speedValue;
    String unitsString;

    if (units == Framework.UNITS_IMPERIAL)
    {
      speedValue = MpsToMiph(speedInMetersPerSecond);
      unitsString = mUnitStringMiph;
    }
    else
    {
      speedValue = MpsToKmph(speedInMetersPerSecond);
      unitsString = mUnitStringKmh;
    }

    String speedString;

    if (speedValue < 10.0)
    {
      speedString = Long.toString(Math.round(speedValue * 10.0));

      StringBuffer buffer = new StringBuffer(speedString);

      if (mDecimalSeparator == Character.MIN_VALUE)
        mDecimalSeparator = DecimalFormatSymbols.getInstance().getDecimalSeparator();

      // For low values (< 1.0), force to have 2 characters in string.
      if (buffer.length() < 2)
        buffer.insert(0, "0");

      buffer.insert(1, mDecimalSeparator);

      speedString = buffer.toString();
    }
    else
      speedString = Long.toString(Math.round(speedValue));

    return new Pair<>(speedString, unitsString);
  }
}
