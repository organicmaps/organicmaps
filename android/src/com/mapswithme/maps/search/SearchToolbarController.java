package com.mapswithme.maps.search;

import android.app.Activity;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.util.UiUtils;

public class SearchToolbarController implements OnClickListener
{
  private final Activity mActivity;

  private final TextView mQuery;
  private final View mProgress;
  private final View mVoiceInput;
  private final Toolbar mToolbar;

  private static String sSavedQuery = "";


  public SearchToolbarController(Activity activity)
  {
    mActivity = activity;
    mToolbar = (Toolbar) mActivity.findViewById(R.id.toolbar_search);
    UiUtils.showHomeUpButton(mToolbar);
    mToolbar.setNavigationOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        MwmActivity.startSearch(mActivity, mQuery.getText().toString());
        cancelSearchApiAndHide();
      }
    });
    View searchBox = mToolbar.findViewById(R.id.search_box);
    mQuery = (TextView) searchBox.findViewById(R.id.search_text_query);
    mProgress = searchBox.findViewById(R.id.search_progress);
    mVoiceInput = searchBox.findViewById(R.id.search_voice_input);

    mQuery.setOnClickListener(this);
    searchBox.findViewById(R.id.search_image_clear).setOnClickListener(this);
  }

  public void refreshToolbar()
  {
    mQuery.setFocusable(false);
    UiUtils.hide(mProgress, mVoiceInput);

    if (ParsedMwmRequest.hasRequest())
    {
      UiUtils.appearSlidingDown(mToolbar, null);
      mQuery.setText(ParsedMwmRequest.getCurrentRequest().getTitle());
    }
    else if (!TextUtils.isEmpty(sSavedQuery))
    {
      UiUtils.appearSlidingDown(mToolbar, null);
      mQuery.setText(sSavedQuery);
    }
    else
      hide();
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (R.id.search_text_query == id)
    {
      final String query = mQuery.getText().toString();
      MwmActivity.startSearch(mActivity, query);
      hide();
    }
    else if (R.id.search_image_clear == id)
      cancelSearchApiAndHide();
    else
      throw new IllegalArgumentException("Unknown id");
  }

  public static void setQuery(String query)
  {
    sSavedQuery = (query == null) ? "" : query;
  }

  public static String getQuery()
  {
    return sSavedQuery;
  }

  public static void cancelApiCall()
  {
    if (ParsedMwmRequest.hasRequest())
      ParsedMwmRequest.setCurrentRequest(null);
    Framework.nativeClearApiPoints();
  }

  public static void cancelSearch()
  {
    setQuery(null);
    Framework.cleanSearchLayerOnMap();
  }

  private void cancelSearchApiAndHide()
  {
    cancelApiCall();
    cancelSearch();
    mQuery.setText(null);
    hide();
  }

  public boolean hide() {
    if (mToolbar.getVisibility() != View.VISIBLE)
      return false;

    UiUtils.disappearSlidingUp(mToolbar, null);
    return true;
  }

  public Toolbar getToolbar()
  {
    return mToolbar;
  }
}
