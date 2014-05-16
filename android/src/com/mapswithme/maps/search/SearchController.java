package com.mapswithme.maps.search;

import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.SearchActivity;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.util.UiUtils;

public class SearchController implements OnClickListener
{
  private MWMActivity mMapActivity;

  // Views
  private TextView mSearchQueryTV;
  private View mClearView;
  private View mSearchProgress;
  private View mVoiceInput;

  // Data
  private String mQuery = "";

  // Singlton
  // No threadsafety needed as everything goes on UI
  private static SearchController mInstance = null;

  public static SearchController get()
  {
    if (mInstance == null)
      mInstance = new SearchController();

    return mInstance;
  }

  private SearchController()
  {}

  public void onCreate(MWMActivity mwmActivity)
  {
    mMapActivity = mwmActivity;

    mSearchQueryTV = (TextView) mMapActivity.findViewById(R.id.search_text_query);
    mClearView = mMapActivity.findViewById(R.id.search_image_clear);
    mSearchProgress = mMapActivity.findViewById(R.id.search_progress);
    mVoiceInput = mMapActivity.findViewById(R.id.search_voice_input);

    mSearchQueryTV.setOnClickListener(this);
    mClearView.setOnClickListener(this);
  }

  public void onResume()
  {
    if (ParsedMmwRequest.hasRequest())
      mSearchQueryTV.setText(ParsedMmwRequest.getCurrentRequest().getTitle());
    else
      mSearchQueryTV.setText(getQuery());

    mSearchQueryTV.setFocusable(false);
    UiUtils.hide(mSearchProgress);
    UiUtils.hide(mVoiceInput);

    UiUtils.showIf(!TextUtils.isEmpty(mSearchQueryTV.getText()), mClearView);
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (R.id.search_text_query == id)
    {
      final String query = mSearchQueryTV.getText().toString();
      SearchActivity.startForSearch(mMapActivity, query);
    }
    else if (R.id.search_image_clear == id)
    {
      // Clear API points first, then clear additional layer
      // (Framework::Invalidate is called inside).
      //cancelApiCall(); // temporary comment for see search result and api points on map

      Framework.cleanSearchLayerOnMap();

      mSearchQueryTV.setText(null);
      UiUtils.hide(mClearView);
    }
    else
      throw new IllegalArgumentException("Unknown id");
  }

  public void cancelApiCall()
  {
    if (ParsedMmwRequest.hasRequest())
    {
      ParsedMmwRequest.setCurrentRequest(null);
      Framework.clearApiPoints();
    }
  }

  public void setQuery(String query)
  {
    mQuery = query == null ? "" : query;
  }

  public String getQuery()
  {
    return mQuery;
  }

  public void cancel()
  {
    setQuery(null);
    Framework.cleanSearchLayerOnMap();
  }
}
