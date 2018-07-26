package com.mapswithme.util;

public class MathUtils
{

  public static double average(double... vals)
  {
    double sum = 0;
    for (double val : vals)
      sum += val;
    return sum / vals.length;
  }

  public static int sum(int... values)
  {
    int total = 0;
    for (int each : values)
    {
      total += each;
    }
    return total;
  }
}
