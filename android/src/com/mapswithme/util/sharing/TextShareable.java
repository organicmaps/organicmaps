package com.mapswithme.util.sharing;

import android.app.Activity;

public class TextShareable extends BaseShareable
{
  public TextShareable(Activity context)
  {
    super(context);
  }

  public TextShareable(Activity context, String text)
  {
    super(context);
    setText(text);
  }

  @Override
  public String getMimeType()
  {
    return ShareAction.TYPE_TEXT_PLAIN;
  }
}
