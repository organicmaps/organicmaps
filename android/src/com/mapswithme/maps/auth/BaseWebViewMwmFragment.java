package com.mapswithme.maps.auth;

import androidx.annotation.IdRes;
import android.view.View;
import android.webkit.WebView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.util.Utils;

public class BaseWebViewMwmFragment extends BaseMwmFragment
{
  @Override
  public boolean onBackPressed()
  {
    View root;
    WebView webView = null;
    boolean goBackAllowed = (root = getView()) != null
                && (webView = Utils.castTo(root.findViewById(getWebViewResId()))) != null
                && webView.canGoBack();
    if (goBackAllowed)
      webView.goBack();
    return goBackAllowed;
  }

  @IdRes
  protected int getWebViewResId()
  {
    return R.id.webview;
  }
}
