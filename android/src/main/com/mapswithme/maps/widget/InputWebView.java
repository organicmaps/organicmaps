package com.mapswithme.maps.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.webkit.WebView;

/**
 * Workaround for not appearing soft keyboard in webview.
 * Check bugreport at https://code.google.com/p/android/issues/detail?id=7189 for more details.
 */
public class InputWebView extends WebView
{
  public InputWebView(Context context)
  {
    super(context);
  }

  public InputWebView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  public InputWebView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
  }

  @Override
  public boolean onCheckIsTextEditor()
  {
    return true;
  }
}
