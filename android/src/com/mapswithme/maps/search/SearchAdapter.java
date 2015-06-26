package com.mapswithme.maps.search;

import android.content.res.Resources;
import android.graphics.Typeface;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

public class SearchAdapter extends BaseAdapter
{
  private static final int mCategoriesIds[] = {
      R.string.food,
      R.string.hotel,
      R.string.tourism,
      R.string.wifi,
      R.string.transport,
      R.string.fuel,
      R.string.parking,
      R.string.shop,
      R.string.atm,
      R.string.bank,
      R.string.entertainment,
      R.string.hospital,
      R.string.pharmacy,
      R.string.police,
      R.string.toilet,
      R.string.post
  };
  private static final int mIcons[] = {
      R.drawable.ic_food,
      R.drawable.ic_hotel,
      R.drawable.ic_tourism,
      R.drawable.ic_wifi,
      R.drawable.ic_transport,
      R.drawable.ic_gas,
      R.drawable.ic_parking,
      R.drawable.ic_shop,
      R.drawable.ic_atm,
      R.drawable.ic_bank,
      R.drawable.ic_entertainment,
      R.drawable.ic_hospital,
      R.drawable.ic_pharmacy,
      R.drawable.ic_police,
      R.drawable.ic_toilet,
      R.drawable.ic_post
  };

  private static final int CATEGORY_TYPE = 0;
  private static final int RESULT_TYPE = 1;
  private static final int MESSAGE_TYPE = 2;
  private static final int VIEW_TYPE_COUNT = 3;
  private final SearchFragment mSearchFragment;
  private final LayoutInflater mInflater;
  private final Resources mResources;

  private static final int COUNT_NO_RESULTS = -1;
  private int mCount = COUNT_NO_RESULTS;

  private int mResultId;

  public SearchAdapter(SearchFragment fragment)
  {
    mSearchFragment = fragment;
    mInflater = mSearchFragment.getActivity().getLayoutInflater();
    mResources = mSearchFragment.getResources();
  }

  @Override
  public boolean isEnabled(int position)
  {
    return (mSearchFragment.doShowCategories() || getCount() > 0);
  }

  @Override
  public int getItemViewType(int position)
  {
    if (mSearchFragment.doShowCategories())
      return CATEGORY_TYPE;
    else if (position == 0 && doShowSearchOnMapButton())
      return MESSAGE_TYPE;
    else
      return RESULT_TYPE;
  }

  @Override
  public int getViewTypeCount()
  {
    return VIEW_TYPE_COUNT;
  }

  @Override
  public int getCount()
  {
    if (mSearchFragment.doShowCategories())
      return mCategoriesIds.length;
    else if (mCount == COUNT_NO_RESULTS)
      return 0;
    else if (doShowSearchOnMapButton())
      return mCount + 1;
    else
      return mCount;
  }

  private boolean doShowSearchOnMapButton()
  {
    if (mCount == 0)
      return true;

    final SearchResult result = mSearchFragment.getResult(0, mResultId);
    return result != null && result.mType != SearchResult.TYPE_SUGGESTION;
  }

  public int getPositionInResults(int position)
  {
    if (doShowSearchOnMapButton())
      return position - 1;
    else
      return position;
  }

  @Override
  public Object getItem(int position)
  {
    return position;
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    ViewHolder holder;
    final int viewType = getItemViewType(position);
    if (convertView == null)
    {
      switch (viewType)
      {
      case CATEGORY_TYPE:
        convertView = mInflater.inflate(R.layout.item_search_category, parent, false);
        break;
      case RESULT_TYPE:
        convertView = mInflater.inflate(R.layout.item_search, parent, false);
        break;
      default:
        convertView = mInflater.inflate(R.layout.item_search_message, parent, false);
        break;
      }
      holder = new ViewHolder(convertView, viewType);

      convertView.setTag(holder);
    }
    else
      holder = (ViewHolder) convertView.getTag();

    switch (viewType)
    {
    case CATEGORY_TYPE:
      bindCategoryView(holder, position);
      break;
    case RESULT_TYPE:
      bindResultView(holder, getPositionInResults(position));
      break;
    case MESSAGE_TYPE:
      bindMessageView(holder);
      break;
    }

    return convertView;
  }

  private void bindResultView(ViewHolder holder, int position)
  {
    final SearchResult result = mSearchFragment.getResult(position, mResultId);
    if (result != null)
    {
      String country = null;
      String dist = null;
      SpannableStringBuilder builder = new SpannableStringBuilder(result.mName);
      if (result.mType == SearchResult.TYPE_FEATURE)
      {
        if (result.mHighlightRanges.length > 0)
        {
          int j = 0, n = result.mHighlightRanges.length / 2;

          for (int i = 0; i < n; ++i)
          {
            int start = result.mHighlightRanges[j++];
            int len = result.mHighlightRanges[j++];

            builder.setSpan(new StyleSpan(Typeface.BOLD), start, start + len, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
          }
        }
        else
          builder.clearSpans();

        country = result.mCountry;
        dist = result.mDistance;
      }
      else
        builder.setSpan(new ForegroundColorSpan(mResources.getColor(R.color.text_green)), 0, builder.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

      UiUtils.hide(holder.mImageLeft);
      UiUtils.setTextAndShow(holder.mName, builder);
      UiUtils.setTextAndHideIfEmpty(holder.mCountry, country);
      UiUtils.setTextAndHideIfEmpty(holder.mItemType, result.mAmenity);
      UiUtils.setTextAndHideIfEmpty(holder.mDistance, dist);
    }
  }

  private void bindCategoryView(ViewHolder holder, int position)
  {
    UiUtils.setTextAndShow(holder.mName, mResources.getString(mCategoriesIds[position]));
    holder.mImageLeft.setImageResource(mIcons[position]);
  }

  private void bindMessageView(ViewHolder holder)
  {
    UiUtils.setTextAndShow(holder.mName, mResources.getString(R.string.search_on_map));
  }

  /**
   * Update list data.
   *
   * @param count
   * @param resultId
   */
  public void showData(int count, int resultId)
  {
    mCount = count;
    mResultId = resultId;

    notifyDataSetChanged();
  }

  public void showCategories()
  {
    mCount = COUNT_NO_RESULTS;
    notifyDataSetChanged();
  }

  /**
   * Show tapped country or suggestion or category to search.
   *
   * @param position
   * @return Suggestion string with space in the end (for full match purpose).
   */
  public String onItemClick(int position)
  {
    switch (getItemViewType(position))
    {
    case MESSAGE_TYPE:
      Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_ON_MAP_CLICKED);
      return null;
    case RESULT_TYPE:
      final int resIndex = getPositionInResults(position);
      final SearchResult r = mSearchFragment.getResult(resIndex, mResultId);
      if (r != null)
      {
        if (r.mType == SearchResult.TYPE_FEATURE)
        {
          // show country and close activity
          SearchFragment.nativeShowItem(resIndex);
          return null;
        }
        else
        {
          // advise suggestion
          return r.mSuggestion;
        }
      }
      break;
    case CATEGORY_TYPE:
      Statistics.INSTANCE.trackSearchCategoryClicked(mResources.getResourceEntryName(mCategoriesIds[position]));

      return mResources.getString(mCategoriesIds[position]) + ' ';
    }

    return null;
  }

  private static class ViewHolder
  {
    public View mView;
    public TextView mName;
    public TextView mCountry;
    public TextView mDistance;
    public TextView mItemType;
    public ImageView mImageLeft;

    public ViewHolder(View v, int type)
    {
      mView = v;
      mName = (TextView) v.findViewById(R.id.tv__search_category);

      switch (type)
      {
      case CATEGORY_TYPE:
        mImageLeft = (ImageView) v.findViewById(R.id.iv__search_category);
        break;
      case RESULT_TYPE:
        mImageLeft = (ImageView) v.findViewById(R.id.iv__search_image);
        mDistance = (TextView) v.findViewById(R.id.tv_search_distance);
        mCountry = (TextView) v.findViewById(R.id.tv_search_item_subtitle);
        mItemType = (TextView) v.findViewById(R.id.tv_search_item_type);
        break;
      case MESSAGE_TYPE:
        mImageLeft = (ImageView) v.findViewById(R.id.iv__search_image);
        mCountry = (TextView) v.findViewById(R.id.tv_search_item_subtitle);
        break;
      }
    }
  }

  // Created from native code.
  public static class SearchResult
  {
    public String mName;
    public String mSuggestion;
    public String mCountry;
    public String mAmenity;
    public String mDistance;

    // 0 - suggestion result
    // 1 - feature result
    public static final int TYPE_SUGGESTION = 0;
    public static final int TYPE_FEATURE = 1;

    public int mType;
    // consecutive pairs of numbers (each pair contains : start index, length), specifying highlighted substrings of original query
    public int[] mHighlightRanges;


    // Called from native code
    @SuppressWarnings("unused")
    public SearchResult(String name, String suggestion, int[] highlightRanges)
    {
      mName = name;
      mSuggestion = suggestion;
      mType = TYPE_SUGGESTION;

      mHighlightRanges = highlightRanges;
    }

    // Called from native code
    @SuppressWarnings("unused")
    public SearchResult(String name, String country, String amenity,
                        String distance, int[] highlightRanges)
    {
      mName = name;
      mCountry = country;
      mAmenity = amenity;
      mDistance = distance;

      mType = TYPE_FEATURE;

      mHighlightRanges = highlightRanges;
    }
  }
}