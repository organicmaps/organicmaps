package com.mapswithme.maps.search;

import android.app.Activity;
import android.text.TextUtils;
import android.view.View;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.UiUtils;

public class FloatingSearchToolbarController extends SearchToolbarController
{
  public FloatingSearchToolbarController(Activity activity)
  {
    super(activity.getWindow().getDecorView(), activity);
  }

  @Override
  protected void onUpClick()
  {
    ((MwmActivity) mActivity).showSearch(getQuery());
    cancelSearchApiAndHide(true);
  }

  @Override
  protected void onQueryClick(String query)
  {
    super.onQueryClick(query);
    ((MwmActivity) mActivity).showSearch(getQuery());
    hide();
  }

  @Override
  protected void onClearClick()
  {
    super.onClearClick();
    cancelSearchApiAndHide(false);
  }

  public void refreshToolbar()
  {
    showProgress(false);

    if (ParsedMwmRequest.hasRequest())
    {
      UiUtils.appearSlidingDown(mToolbar, null);
      setQuery(ParsedMwmRequest.getCurrentRequest().getTitle());
    }
    else if (!TextUtils.isEmpty(SearchEngine.getQuery()))
    {
      UiUtils.appearSlidingDown(mToolbar, null);
      setQuery(SearchEngine.getQuery());
    }
    else
    {
      hide();
      clear();
    }
  }

  private void cancelSearchApiAndHide(boolean clearText)
  {
    SearchEngine.cancelApiCall();
    SearchEngine.cancelSearch();

    if (clearText)
      clear();

    hide();
  }

  public boolean hide()
  {
    if (mToolbar.getVisibility() != View.VISIBLE)
      return false;

    UiUtils.disappearSlidingUp(mToolbar, null);
    return true;
  }
}
