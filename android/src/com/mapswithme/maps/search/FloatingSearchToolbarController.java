package com.mapswithme.maps.search;

import android.app.Activity;
import android.text.TextUtils;
import android.view.View;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.UiUtils;

public class FloatingSearchToolbarController extends SearchToolbarController
{
  private static String sSavedQuery = "";

  public FloatingSearchToolbarController(Activity activity)
  {
    super(activity.getWindow().getDecorView(), activity);
  }

  @Override
  protected void onUpClick()
  {
    MwmActivity.startSearch(mActivity, getQuery());
    cancelSearchApiAndHide(true);
  }

  @Override
  protected void onQueryClick(String query)
  {
    super.onQueryClick(query);

    MwmActivity.startSearch(mActivity, query);
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
    else if (!TextUtils.isEmpty(sSavedQuery))
    {
      UiUtils.appearSlidingDown(mToolbar, null);
      setQuery(sSavedQuery);
    }
    else
      hide();
  }

  public static void saveQuery(String query)
  {
    sSavedQuery = (query == null) ? "" : query;
  }

  public static void cancelApiCall()
  {
    if (ParsedMwmRequest.hasRequest())
      ParsedMwmRequest.setCurrentRequest(null);
    Framework.nativeClearApiPoints();
  }

  public static void cancelSearch()
  {
    saveQuery(null);
    Framework.cleanSearchLayerOnMap();
  }

  private void cancelSearchApiAndHide(boolean clearText)
  {
    cancelApiCall();
    cancelSearch();

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
