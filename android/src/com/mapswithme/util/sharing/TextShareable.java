package com.mapswithme.util.sharing;

import android.app.Activity;

public class TextShareable extends BaseShareable
{
  public TextShareable(Activity context, String text)
  {
    super(context);
    setText(text);
  }

  @Override
  public String getMimeType()
  {
    return TargetUtils.TYPE_TEXT_PLAIN;
  }
}
