package app.organicmaps.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.webkit.WebView;

public class ObservableWebView extends WebView
{
  public interface Listener
  {
    void onScroll(int left, int top);
    void onContentReady();
  }

  private Listener mListener;
  private boolean mContentReady;

  public ObservableWebView(Context context)
  {
    super(context);
  }

  public ObservableWebView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  public ObservableWebView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
  }

  public void setListener(Listener listener)
  {
    mListener = listener;
  }

  @Override
  protected void onScrollChanged(int left, int top, int oldLeft, int oldTop)
  {
    super.onScrollChanged(left, top, oldLeft, oldTop);
    if (mListener != null)
      mListener.onScroll(left, top);
  }

  @Override
  public void invalidate()
  {
    super.invalidate();
    if (!mContentReady && getContentHeight() > 0)
    {
      mContentReady = true;
      if (mListener != null)
        mListener.onContentReady();
    }
  }
}
