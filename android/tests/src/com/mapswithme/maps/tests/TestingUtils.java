package com.mapswithme.maps.tests;

import android.widget.TextView;
import junit.framework.Assert;

public class TestingUtils
{

  public static void assertTextViewHasText(String expected, TextView textView)
  {
    Assert.assertEquals(expected, textView.getText().toString());
  }


  private TestingUtils() {};
}
