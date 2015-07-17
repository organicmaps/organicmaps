package com.mapswithme.maps.search;

import android.content.res.Resources;
import android.graphics.Typeface;
import android.support.v7.widget.RecyclerView;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

public class SearchAdapter extends RecyclerView.Adapter<SearchAdapter.ViewHolder>
{
  private static final int RESULT_TYPE = 0;
  private static final int MESSAGE_TYPE = 1;
  private final SearchFragment mSearchFragment;
  private final LayoutInflater mInflater;
  private final Resources mResources;

  private static final int COUNT_NO_RESULTS = -1;
  private int mResultsCount = COUNT_NO_RESULTS;
  private int mResultsId;

  public SearchAdapter(SearchFragment fragment)
  {
    mSearchFragment = fragment;
    mInflater = mSearchFragment.getActivity().getLayoutInflater();
    mResources = mSearchFragment.getResources();
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    if (viewType == RESULT_TYPE)
      return new ViewHolder(mInflater.inflate(R.layout.item_search, parent, false), viewType);

    return new ViewHolder(mInflater.inflate(R.layout.item_search_message, parent, false), viewType);
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    if (holder.getItemViewType() == RESULT_TYPE)
      bindResultView(holder);
    else
      bindMessageView(holder);
  }

  @Override
  public int getItemViewType(int position)
  {
    if (position == 0 && doShowSearchOnMapButton())
      return MESSAGE_TYPE;

    return RESULT_TYPE;
  }

  private boolean doShowSearchOnMapButton()
  {
    if (mResultsCount == 0)
      return true;

    final SearchResult result = mSearchFragment.getResult(0, mResultsId);
    return result != null && result.mType != SearchResult.TYPE_SUGGESTION;
  }

  public int getPositionInResults(int position)
  {
    if (doShowSearchOnMapButton())
      return position - 1;

    return position;
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public int getItemCount()
  {
    if (mResultsCount == COUNT_NO_RESULTS)
      return 0;
    else if (doShowSearchOnMapButton())
      return mResultsCount + 1;

    return mResultsCount;
  }

  private void bindResultView(ViewHolder holder)
  {
    final int position = getPositionInResults(holder.getAdapterPosition());
    final SearchResult result = mSearchFragment.getResult(position, mResultsId);
    if (result != null)
    {
      SpannableStringBuilder builder = new SpannableStringBuilder(result.mName);
      if (result.mHighlightRanges != null && result.mHighlightRanges.length > 0)
      {
        int j = 0, n = result.mHighlightRanges.length / 2;

        for (int i = 0; i < n; ++i)
        {
          int start = result.mHighlightRanges[j++];
          int len = result.mHighlightRanges[j++];

          builder.setSpan(new StyleSpan(Typeface.BOLD), start, start + len, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
      }

      if (result.mType == SearchResult.TYPE_SUGGESTION)
      {
        builder.setSpan(new ForegroundColorSpan(mResources.getColor(R.color.text_search_suggestion)), 0, builder.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        UiUtils.hide(holder.mCountry, holder.mDistance);
      }
      else
      {
        UiUtils.setTextAndHideIfEmpty(holder.mCountry, result.mCountry);
        UiUtils.setTextAndHideIfEmpty(holder.mDistance, result.mDistance);
      }

      UiUtils.setTextAndShow(holder.mName, builder);
      UiUtils.setTextAndHideIfEmpty(holder.mType, result.mAmenity);
    }
  }

  private void bindMessageView(ViewHolder holder)
  {
    UiUtils.setTextAndShow(holder.mName, mResources.getString(R.string.search_on_map));
  }

  /**
   * Update list data.
   *
   * @param count    total count of result
   * @param resultId id to query results
   */
  public void refreshData(int count, int resultId)
  {
    mResultsCount = count;
    mResultsId = resultId;

    notifyDataSetChanged();
  }

  public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
    public View mView;
    public TextView mName;
    public TextView mCountry;
    public TextView mDistance;
    public TextView mType;

    public ViewHolder(View v, int type)
    {
      super(v);

      mView = v;
      mView.setOnClickListener(this);
      if (type == MESSAGE_TYPE)
        mName = (TextView) mView;
      else
      {
        mName = (TextView) v.findViewById(R.id.tv__search_title);
        mCountry = (TextView) v.findViewById(R.id.tv__search_subtitle);
        mDistance = (TextView) v.findViewById(R.id.tv__search_distance);
        mType = (TextView) v.findViewById(R.id.tv__search_type);
      }
    }

    @Override
    public void onClick(View v)
    {
      if (getItemViewType() == MESSAGE_TYPE)
      {
        Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_ON_MAP_CLICKED);
        mSearchFragment.showAllResultsOnMap();
      }
      else
      {
        final int resIndex = getPositionInResults(getAdapterPosition());
        final SearchResult result = mSearchFragment.getResult(resIndex, mResultsId);
        if (result != null)
        {
          if (result.mType == SearchResult.TYPE_FEATURE)
            mSearchFragment.showSearchResultOnMap(resIndex);
          else
            mSearchFragment.setSearchQuery(result.mSuggestion);
        }
      }
    }
  }
}