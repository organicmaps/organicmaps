package com.mapswithme.maps.search;

import android.app.Activity;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.util.UiUtils;

public class SearchController implements OnClickListener
{
  private Activity mActivity;
  // Views
  private TextView mSearchQueryTV;
  private View mClearView;
  private View mSearchProgress;
  private View mVoiceInput;
  private ViewGroup mSearchBox;
  private Toolbar mSearchToolbar;

  // Data
  private String mQuery = "";

  // No threadsafety needed as everything goes on UI
  private static SearchController sInstance = null;

  // TODO remove view members from static instance. or remove static-ness & singleton.
  public static SearchController getInstance()
  {
    if (sInstance == null)
      sInstance = new SearchController();

    return sInstance;
  }

  private SearchController()
  {}

  public void onCreate(MWMActivity activity)
  {
    mActivity = activity;
    mSearchToolbar = (Toolbar) activity.findViewById(R.id.toolbar_search);
    UiUtils.showHomeUpButton(mSearchToolbar);
    mSearchToolbar.setNavigationOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        exitSearchAndApi();
      }
    });
    mSearchBox = (ViewGroup) mSearchToolbar.findViewById(R.id.search_box);
    mSearchQueryTV = (TextView) mSearchBox.findViewById(R.id.search_text_query);
    mClearView = mSearchBox.findViewById(R.id.search_image_clear);
    mSearchProgress = mSearchBox.findViewById(R.id.search_progress);
    mVoiceInput = mSearchBox.findViewById(R.id.search_voice_input);

    mSearchQueryTV.setOnClickListener(this);
    mClearView.setOnClickListener(this);
  }

  public void onResume()
  {
    mSearchQueryTV.setFocusable(false);
    UiUtils.hide(mSearchProgress);
    UiUtils.hide(mVoiceInput);

    if (ParsedMmwRequest.hasRequest())
    {
      UiUtils.show(mSearchToolbar);
      mSearchQueryTV.setText(ParsedMmwRequest.getCurrentRequest().getTitle());
    }
    else if (!TextUtils.isEmpty(mQuery))
    {
      UiUtils.show(mSearchToolbar);
      mSearchQueryTV.setText(mQuery);
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
      final String query = mSearchQueryTV.getText().toString();
      MWMActivity.startSearch(mActivity, query);
      UiUtils.hide(mSearchToolbar);
    }
    else if (R.id.search_image_clear == id)
      exitSearchAndApi();
    else
      throw new IllegalArgumentException("Unknown id");
  }

  public void cancelApiCall()
  {
    if (ParsedMmwRequest.hasRequest())
      ParsedMmwRequest.setCurrentRequest(null);
    Framework.nativeClearApiPoints();
  }

  public void setQuery(String query)
  {
    mQuery = (query == null) ? "" : query;
  }

  public String getQuery()
  {
    return mQuery;
  }

  public void cancelSearch()
  {
    setQuery(null);
    Framework.cleanSearchLayerOnMap();
  }

  private void exitSearchAndApi()
  {
    cancelApiCall();
    cancelSearch();
    mSearchQueryTV.setText(null);
    UiUtils.hide(mSearchToolbar);
  }
}
