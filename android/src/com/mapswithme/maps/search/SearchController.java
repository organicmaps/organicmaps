package com.mapswithme.maps.search;

import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
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
  private ViewGroup mSearchBox;

  // Data
  private String mQuery = "";

  // No threadsafety needed as everything goes on UI
  private static SearchController sInstance = null;

  public static SearchController getInstance()
  {
    if (sInstance == null)
      sInstance = new SearchController();

    return sInstance;
  }

  private SearchController()
  {}

  public void onCreate(MWMActivity mwmActivity)
  {
    mMapActivity = mwmActivity;

    mSearchBox = (ViewGroup) mMapActivity.findViewById(R.id.search_box);
    mSearchQueryTV = (TextView) mSearchBox.findViewById(R.id.search_text_query);
    mClearView = mSearchBox.findViewById(R.id.search_image_clear);
    mSearchProgress = mSearchBox.findViewById(R.id.search_progress);
    mVoiceInput = mSearchBox.findViewById(R.id.search_voice_input);

    mSearchQueryTV.setOnClickListener(this);
    mClearView.setOnClickListener(this);
    UiUtils.hide(mSearchBox.findViewById(R.id.btn_cancel_search));
  }

  public void onResume()
  {
    mSearchQueryTV.setFocusable(false);
    UiUtils.hide(mSearchProgress);
    UiUtils.hide(mVoiceInput);

    if (ParsedMmwRequest.hasRequest())
    {
      UiUtils.show(mSearchBox);
      mSearchQueryTV.setText(ParsedMmwRequest.getCurrentRequest().getTitle());
    }
    else if (!TextUtils.isEmpty(mQuery))
    {
      UiUtils.show(mSearchBox);
      mSearchQueryTV.setText(mQuery);
    }
    else
      UiUtils.hide(mSearchBox);
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
      cancelApiCall();
      cancel();
      mSearchQueryTV.setText(null);
      UiUtils.hide(mSearchBox);
    }
    else
      throw new IllegalArgumentException("Unknown id");
  }

  public void cancelApiCall()
  {
    if (ParsedMmwRequest.hasRequest())
    {
      ParsedMmwRequest.setCurrentRequest(null);
    }
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

  public void cancel()
  {
    setQuery(null);
    Framework.cleanSearchLayerOnMap();
  }
}
