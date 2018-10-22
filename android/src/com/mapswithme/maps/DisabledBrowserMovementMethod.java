package com.mapswithme.maps;

import android.content.ActivityNotFoundException;
import android.text.Spannable;
import android.text.method.LinkMovementMethod;
import android.view.MotionEvent;
import android.widget.TextView;
import android.widget.Toast;

public class DisabledBrowserMovementMethod extends LinkMovementMethod
{
  private DisabledBrowserMovementMethod()
  {
  }

  public static DisabledBrowserMovementMethod getInstance()
  {
    return new DisabledBrowserMovementMethod();
  }

  @Override
  public boolean onTouchEvent(TextView widget, Spannable buffer, MotionEvent event)
  {
    boolean result = true;
    try
    {
      result = super.onTouchEvent(widget, buffer, event);
    }
    catch (ActivityNotFoundException e)
    {
      /* FIXME */
      Toast.makeText(widget.getContext(), R.string.error_system_message, Toast.LENGTH_SHORT).show();
    }
    return result;
  }
}
