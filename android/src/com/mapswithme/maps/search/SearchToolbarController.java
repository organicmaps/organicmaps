package com.mapswithme.maps.search;

import android.app.Activity;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.util.UiUtils;

public class SearchToolbarController implements OnClickListener
{
  private Activity mActivity;

  private TextView mSearchQuery;
  private View mSearchProgress;
  private View mVoiceInput;
  private Toolbar mSearchToolbar;

  private static String mQuery = "";

  public SearchToolbarController(Activity activity)
  {
    mActivity = activity;
    mSearchToolbar = (Toolbar) mActivity.findViewById(R.id.toolbar_search);
    UiUtils.showHomeUpButton(mSearchToolbar);
    mSearchToolbar.setNavigationOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        cancelSearchApiAndHide();
      }
    });
    View searchBox = mSearchToolbar.findViewById(R.id.search_box);
    mSearchQuery = (TextView) searchBox.findViewById(R.id.search_text_query);
    mSearchProgress = searchBox.findViewById(R.id.search_progress);
    mVoiceInput = searchBox.findViewById(R.id.search_voice_input);

    mSearchQuery.setOnClickListener(this);
    searchBox.findViewById(R.id.search_image_clear).setOnClickListener(this);
  }

  public void refreshToolbar()
  {
    mSearchQuery.setFocusable(false);
    UiUtils.hide(mSearchProgress, mVoiceInput);

    if (ParsedMmwRequest.hasRequest())
    {
      UiUtils.show(mSearchToolbar);
      mSearchQuery.setText(ParsedMmwRequest.getCurrentRequest().getTitle());
    }
    else if (!TextUtils.isEmpty(mQuery))
    {
      UiUtils.show(mSearchToolbar);
      mSearchQuery.setText(mQuery);
    }
    else
      UiUtils.hide(mSearchToolbar);
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (R.id.search_text_query == id)
    {
      final String query = mSearchQuery.getText().toString();
      MWMActivity.startSearch(mActivity, query);
      UiUtils.hide(mSearchToolbar);
    }
    else if (R.id.search_image_clear == id)
      cancelSearchApiAndHide();
    else
      throw new IllegalArgumentException("Unknown id");
  }

  public static void setQuery(String query)
  {
    mQuery = (query == null) ? "" : query;
  }

  public static String getQuery()
  {
    return mQuery;
  }

  public static void cancelApiCall()
  {
    if (ParsedMmwRequest.hasRequest())
      ParsedMmwRequest.setCurrentRequest(null);
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
    mSearchQuery.setText(null);
    UiUtils.hide(mSearchToolbar);
  }
}
