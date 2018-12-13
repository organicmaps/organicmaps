package com.mapswithme.maps.widget;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.webkit.WebView;

public class WebViewCompat extends WebView
{
  @Nullable
  private OnScrollChangedListener mOnScrollChangedListener;

  public WebViewCompat(final Context context)
  {
    super(context);
  }

  public WebViewCompat(final Context context, final AttributeSet attrs)
  {
    super(context, attrs);
  }

  public WebViewCompat(final Context context, final AttributeSet attrs, final int defStyle)
  {
    super(context, attrs, defStyle);
  }

  public WebViewCompat(Context context, AttributeSet attrs, int defStyleAttr,
                       boolean privateBrowsing)
  {
    super(context, attrs, defStyleAttr, privateBrowsing);
  }

  @Override
  protected void onScrollChanged(final int l, final int t, final int oldl, final int oldt)
  {
    super.onScrollChanged(l, t, oldl, oldt);
    if(mOnScrollChangedListener != null)
      mOnScrollChangedListener.onScroll(WebViewCompat.this, l, t, oldl, oldt);
  }

  public void setOnScrollChangedListener(@NonNull OnScrollChangedListener onScrollChangedListener)
  {
    mOnScrollChangedListener = onScrollChangedListener;
  }

  public interface OnScrollChangedListener
  {
    void onScroll(@NonNull WebViewCompat webView, int l, int t, int oldl, int oldt);
  }
}
