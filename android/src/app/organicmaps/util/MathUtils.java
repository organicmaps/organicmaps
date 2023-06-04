package app.organicmaps.util;

public class MathUtils
{

  public static double average(double... vals)
  {
    double sum = 0;
    for (double val : vals)
      sum += val;
    return sum / vals.length;
  }

}
