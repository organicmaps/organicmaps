package com.mapswithme.maps.search;

import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.support.annotation.AttrRes;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

class SearchAdapter extends RecyclerView.Adapter<SearchAdapter.BaseViewHolder>
{
  private static final int TYPE_SUGGEST = 0;
  private static final int TYPE_RESULT = 1;
  private static final int TYPE_LOCAL_ADS_CUSTOMER = 2;

  private final SearchFragment mSearchFragment;
  private SearchResult[] mResults;
  private final Drawable mClosedMarkerBackground;

  static abstract class BaseViewHolder extends RecyclerView.ViewHolder
  {
    SearchResult mResult;
    // Position within search results
    int mOrder;

    BaseViewHolder(View view)
    {
      super(view);
      if (view instanceof TextView)
      {
        int tintAttr = getTintAttr();
        if (tintAttr != 0)
          Graphics.tint((TextView)view, tintAttr);
      }
    }

    @AttrRes int getTintAttr()
    {
      return R.attr.colorAccent;
    }

    void bind(@NonNull SearchResult result, int order)
    {
      mResult = result;
      mOrder = order;
    }
  }

  private static abstract class BaseResultViewHolder extends BaseViewHolder
  {
    BaseResultViewHolder(View view)
    {
      super(view);
      view.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          processClick(mResult, mOrder);
        }
      });
    }

    @Override
    void bind(@NonNull SearchResult result, int order)
    {
      super.bind(result, order);

      SpannableStringBuilder builder = new SpannableStringBuilder(result.name);
      if (result.highlightRanges != null)
      {
        final int size = result.highlightRanges.length / 2;
        int index = 0;

        for (int i = 0; i < size; i++)
        {
          final int start = result.highlightRanges[index++];
          final int len = result.highlightRanges[index++];

          builder.setSpan(new StyleSpan(Typeface.BOLD), start, start + len, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
      }

      getTitleView().setText(builder);
    }

    abstract TextView getTitleView();

    abstract void processClick(SearchResult result, int order);
  }

  private class SuggestViewHolder extends BaseResultViewHolder
  {
    SuggestViewHolder(View view)
    {
      super(view);
    }

    @Override
    TextView getTitleView()
    {
      return (TextView) itemView;
    }

    @Override
    void processClick(SearchResult result, int order)
    {
      mSearchFragment.setQuery(result.suggestion);
    }
  }

  private class ResultViewHolder extends BaseResultViewHolder
  {
    final TextView mName;
    final View mClosedMarker;
    final TextView mDescription;
    final TextView mRegion;
    final TextView mDistance;
    final TextView mPriceCategory;

    @Override
    int getTintAttr()
    {
      return 0;
    }

    // FIXME: Better format based on result type
    private CharSequence formatDescription(SearchResult result)
    {
      final SpannableStringBuilder res = new SpannableStringBuilder(result.description.featureType);
      final SpannableStringBuilder tail = new SpannableStringBuilder();

      final int stars = Math.min(result.description.stars, 5);
      if (stars > 0 || !result.description.rating.isEmpty())
      {
        if (stars > 0)
        {
          // Colorize last dimmed stars
          final SpannableStringBuilder sb = new SpannableStringBuilder("★ ★ ★ ★ ★");
          if (stars < 5)
          {
            final int start = sb.length() - ((5 - stars) * 2 - 1);
            sb.setSpan(new ForegroundColorSpan(itemView.getResources().getColor(R.color.search_star_dimmed)),
                    start, sb.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
          }

          tail.append(" • ");
          tail.append(sb);
        }

        if (!result.description.rating.isEmpty())
        {
          final SpannableStringBuilder sb = new SpannableStringBuilder(
                  itemView.getResources().getString(R.string.place_page_booking_rating, result.description.rating));
          sb.setSpan(new ForegroundColorSpan(itemView.getResources().getColor(R.color.base_green)),
                  0, sb.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);

          tail
            .append(" • ")
            .append(sb);
        }
      }
      else if (!TextUtils.isEmpty(result.description.cuisine))
      {
        tail.append(" • " + result.description.cuisine);
      }

      res.append(tail);

      return res;
    }

    ResultViewHolder(View view)
    {
      super(view);

      mName = (TextView) view.findViewById(R.id.title);
      mClosedMarker = view.findViewById(R.id.closed);
      mDescription = (TextView) view.findViewById(R.id.description);
      mRegion = (TextView) view.findViewById(R.id.region);
      mDistance = (TextView) view.findViewById(R.id.distance);
      mPriceCategory = (TextView) view.findViewById(R.id.price_category);

      mClosedMarker.setBackgroundDrawable(mClosedMarkerBackground);
    }

    @Override
    TextView getTitleView()
    {
      return mName;
    }

    @Override
    void bind(@NonNull SearchResult result, int order)
    {
      super.bind(result, order);

      // TODO: Support also "Open Now" mark.
      UiUtils.showIf(result.description.openNow == SearchResult.OPEN_NOW_NO, mClosedMarker);
      UiUtils.setTextAndHideIfEmpty(mDescription, formatDescription(result));
      UiUtils.setTextAndHideIfEmpty(mRegion, result.description.region);
      UiUtils.setTextAndHideIfEmpty(mDistance, result.description.distance);
      UiUtils.setTextAndHideIfEmpty(mPriceCategory, result.description.pricing);
    }

    @Override
    void processClick(SearchResult result, int order)
    {
      mSearchFragment.showSingleResultOnMap(result, order);
    }
  }

  private class LocalAdsCustomerViewHolder extends ResultViewHolder
  {
    LocalAdsCustomerViewHolder(View view)
    {
      super(view);

      int resId = ThemeUtils.isNightTheme() ? R.drawable.search_la_customer_result_night
                                            : R.drawable.search_la_customer_result;

      view.setBackgroundDrawable(mSearchFragment.getResources().getDrawable(resId));
    }
  }

  SearchAdapter(SearchFragment fragment)
  {
    mSearchFragment = fragment;
    mClosedMarkerBackground = fragment.getResources().getDrawable(ThemeUtils.isNightTheme() ? R.drawable.search_closed_marker_night
                                                                                            : R.drawable.search_closed_marker);
  }

  @Override
  public BaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());

    switch (viewType)
    {
    case TYPE_SUGGEST:
      return new SuggestViewHolder(inflater.inflate(R.layout.item_search_suggest, parent, false));

    case TYPE_RESULT:
      return new ResultViewHolder(inflater.inflate(R.layout.item_search_result, parent, false));

    case TYPE_LOCAL_ADS_CUSTOMER:
      return new LocalAdsCustomerViewHolder(inflater.inflate(R.layout.item_search_result, parent,
                                                             false));

    default:
      throw new IllegalArgumentException("Unhandled view type given");
    }
  }

  @Override
  public void onBindViewHolder(BaseViewHolder holder, int position)
  {
    holder.bind(mResults[position], position);
  }

  @Override
  public int getItemViewType(int position)
  {
    switch (mResults[position].type)
    {
    case SearchResult.TYPE_SUGGEST:
      return TYPE_SUGGEST;

    case SearchResult.TYPE_RESULT:
      return TYPE_RESULT;

    case SearchResult.TYPE_LOCAL_ADS_CUSTOMER:
      return TYPE_LOCAL_ADS_CUSTOMER;

    default:
      throw new IllegalArgumentException("Unhandled SearchResult type");
    }
  }

  boolean showPopulateButton()
  {
    return (!RoutingController.get().isWaitingPoiPick() &&
            mResults != null &&
            mResults.length > 0 &&
            mResults[0].type != SearchResult.TYPE_SUGGEST);
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public int getItemCount()
  {
    int res = 0;
    if (mResults == null)
      return res;

    res += mResults.length;
    return res;
  }

  public void clear()
  {
    refreshData(null);
  }

  void refreshData(SearchResult[] results)
  {
    mResults = results;
    notifyDataSetChanged();
  }
}
