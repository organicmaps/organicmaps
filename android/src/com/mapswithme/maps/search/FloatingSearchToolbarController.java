package com.mapswithme.maps.search;

import android.app.Activity;
import android.text.TextUtils;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.Animations;
import com.mapswithme.util.UiUtils;

public class FloatingSearchToolbarController extends SearchToolbarController
{
  public FloatingSearchToolbarController(Activity activity)
  {
    super(activity.getWindow().getDecorView(), activity);
  }

  @Override
  public void onUpClick()
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
      Animations.appearSliding(mToolbar, Animations.TOP, null);
      setQuery(ParsedMwmRequest.getCurrentRequest().getTitle());
    }
    else if (!TextUtils.isEmpty(SearchEngine.getQuery()))
    {
      Animations.appearSliding(mToolbar, Animations.TOP, null);
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
    if (!UiUtils.isVisible(mToolbar))
      return false;

    Animations.disappearSliding(mToolbar, Animations.TOP, null);
    return true;
  }
}
