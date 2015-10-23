package com.mapswithme.maps.widget;

import com.mapswithme.util.UiUtils;

public class WebViewShadowController extends BaseShadowController<ObservableWebView>
{
  public WebViewShadowController(ObservableWebView webView)
  {
    super(webView);
  }

  @Override
  protected boolean shouldShowShadow(int id)
  {
    switch (id)
    {
      case TOP:
        return (mList.getScrollY() > 0);

      case BOTTOM:
        return (mList.getScrollY() + mList.getHeight() < UiUtils.toPx(mList.getContentHeight() - 1));

      default:
        throw new IllegalArgumentException("Invalid shadow id: " + id);
    }
  }

  @Override
  public BaseShadowController attach()
  {
    super.attach();
    mList.setListener(new ObservableWebView.Listener()
    {
      @Override
      public void onScroll(int left, int top)
      {
        updateShadows();
      }

      @Override
      public void onContentReady()
      {
        updateShadows();
      }
    });

    return this;
  }

  @Override
  public void detach()
  {
    super.detach();
    mList.setListener(null);
  }
}
