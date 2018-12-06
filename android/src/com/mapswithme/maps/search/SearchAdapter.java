package com.mapswithme.maps.search;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.support.annotation.AttrRes;
import android.support.annotation.ColorInt;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import com.mapswithme.HotelUtils;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import static com.mapswithme.util.Constants.Rating.RATING_INCORRECT_VALUE;

class SearchAdapter extends RecyclerView.Adapter<SearchAdapter.SearchDataViewHolder>
{
  private final SearchFragment mSearchFragment;
  private SearchData[] mResults;
  @NonNull
  private final FilteredHotelIds mFilteredHotelIds = new FilteredHotelIds();
  private final Drawable mClosedMarkerBackground;

  static abstract class SearchDataViewHolder extends RecyclerView.ViewHolder
  {
    SearchDataViewHolder(@NonNull View itemView)
    {
      super(itemView);
    }

    abstract void bind(@NonNull SearchData searchData, int position);
  }

  private static abstract class BaseResultViewHolder extends SearchDataViewHolder
  {
    SearchResult mResult;
    // Position within search results
    int mOrder;

    BaseResultViewHolder(@NonNull View view)
    {
      super(view);
      if (view instanceof TextView)
      {
        int tintAttr = getTintAttr();
        if (tintAttr != 0)
          Graphics.tint((TextView)view, tintAttr);
      }
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
    void bind(@NonNull SearchData result, int order)
    {
      mResult = (SearchResult)result;
      mOrder = order;
      TextView titleView = getTitleView();

      String title = mResult.name;
      if (TextUtils.isEmpty(title))
      {
        SearchResult.Description description = mResult.description;
        title = description != null
                ? Utils.getLocalizedFeatureType(titleView.getContext(), description.featureType)
                : "";
      }

      SpannableStringBuilder builder = new SpannableStringBuilder(title);
      if (mResult.highlightRanges != null)
      {
        final int size = mResult.highlightRanges.length / 2;
        int index = 0;

        for (int i = 0; i < size; i++)
        {
          final int start = mResult.highlightRanges[index++];
          final int len = mResult.highlightRanges[index++];

          builder.setSpan(new StyleSpan(Typeface.BOLD), start, start + len, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
      }

      if (titleView != null)
        titleView.setText(builder);
    }

    @AttrRes int getTintAttr()
    {
      return R.attr.colorAccent;
    }

    abstract TextView getTitleView();

    abstract void processClick(SearchResult result, int order);
  }

  private static class FilteredHotelIds
  {
    @NonNull
    private final SparseArray<Set<FeatureId>> mFilteredHotelIds = new SparseArray<>();

    void put(@BookingFilter.Type int type, @NonNull FeatureId[] hotelsId)
    {
      mFilteredHotelIds.put(type, new HashSet<>(Arrays.asList(hotelsId)));
    }

    boolean contains(@BookingFilter.Type int type, @NonNull FeatureId id)
    {
      Set<FeatureId>  ids = mFilteredHotelIds.get(type);

      return ids != null && ids.contains(id);
    }
  }

  private class SuggestViewHolder extends BaseResultViewHolder
  {
    SuggestViewHolder(@NonNull View view)
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

  private static class GoogleAdsViewHolder extends SearchDataViewHolder
  {
    @NonNull
    private ViewGroup container;

    GoogleAdsViewHolder(@NonNull View view)
    {
      super(view);
      container = (FrameLayout)view;
    }

    @Override
    void bind(@NonNull SearchData searchData, int position)
    {
      container.removeAllViews();
      container.addView(((GoogleAdsBanner)searchData).getAdView());
    }
  }

  private class ResultViewHolder extends BaseResultViewHolder
  {
    @NonNull
    final View mFrame;
    @NonNull
    final TextView mName;
    @NonNull
    final View mClosedMarker;
    @NonNull
    final View mPopularity;
    @NonNull
    final TextView mDescription;
    @NonNull
    final TextView mRegion;
    @NonNull
    final TextView mDistance;
    @NonNull
    final TextView mPriceCategory;
    @NonNull
    final View mSale;

    @Override
    int getTintAttr()
    {
      return 0;
    }

    // FIXME: Better format based on result type
    private CharSequence formatDescription(SearchResult result, boolean isHotelAvailable)
    {
      String localizedType = Utils.getLocalizedFeatureType(mFrame.getContext(),
                                                           result.description.featureType);
      final SpannableStringBuilder res = new SpannableStringBuilder(localizedType);
      final SpannableStringBuilder tail = new SpannableStringBuilder();

      int stars = result.description.stars;
      if (stars > 0 || result.description.rating != RATING_INCORRECT_VALUE || isHotelAvailable)
      {
        if (stars > 0)
        {
          tail.append(" • ");
          tail.append(HotelUtils.formatStars(stars, itemView.getResources()));
        }

        if (result.description.rating != RATING_INCORRECT_VALUE)
        {
          Resources rs = itemView.getResources();
          String s = rs.getString(R.string.place_page_booking_rating,
                                  UGC.nativeFormatRating(result.description.rating));
          tail
            .append(" • ")
            .append(colorizeString(s, rs.getColor(R.color.base_green)));
        }

        if (isHotelAvailable)
        {
          Resources rs = itemView.getResources();
          String s = itemView.getResources().getString(R.string.hotel_available);
          if (tail.length() > 0)
            tail.append(" • ");
          tail.append(colorizeString(s, rs.getColor(R.color.base_green)));
        }
      }
      else if (!TextUtils.isEmpty(result.description.airportIata))
      {
        tail.append(" • " + result.description.airportIata);
      }
      else
      {
        if (!TextUtils.isEmpty(result.description.brand))
        {
          tail.append(" • " + Utils.getLocalizedBrand(mFrame.getContext(), result.description.brand));
        }
        if (!TextUtils.isEmpty(result.description.cuisine))
        {
          tail.append(" • " + result.description.cuisine);
        }
      }

      res.append(tail);

      return res;
    }

    @NonNull
    private CharSequence colorizeString(@NonNull String str, @ColorInt int color)
    {
      final SpannableStringBuilder sb = new SpannableStringBuilder(str);
      sb.setSpan(new ForegroundColorSpan(color),
                 0, sb.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
      return sb;
    }

    ResultViewHolder(@NonNull View view)
    {
      super(view);
      mFrame = view;
      mName = view.findViewById(R.id.title);
      mClosedMarker = view.findViewById(R.id.closed);
      mPopularity = view.findViewById(R.id.popular_rating_view);
      mDescription =  view.findViewById(R.id.description);
      mRegion = view.findViewById(R.id.region);
      mDistance = view.findViewById(R.id.distance);
      mPriceCategory = view.findViewById(R.id.price_category);
      mSale = view.findViewById(R.id.sale);

      mClosedMarker.setBackgroundDrawable(mClosedMarkerBackground);
    }

    @Override
    TextView getTitleView()
    {
      return mName;
    }

    @Override
    void bind(@NonNull SearchData result, int order)
    {
      super.bind(result, order);
      setBackground();
      // TODO: Support also "Open Now" mark.

      UiUtils.showIf(isClosedVisible(), mClosedMarker);
      boolean isHotelAvailable = mResult.isHotel &&
                                 mFilteredHotelIds.contains(BookingFilter.TYPE_AVAILABILITY,
                                                            mResult.description.featureId);

      UiUtils.showIf(isPopularVisible(), mPopularity);
      UiUtils.setTextAndHideIfEmpty(mDescription, formatDescription(mResult, isHotelAvailable));
      UiUtils.setTextAndHideIfEmpty(mRegion, mResult.description.region);
      UiUtils.setTextAndHideIfEmpty(mDistance, mResult.description.distance);
      UiUtils.setTextAndHideIfEmpty(mPriceCategory, mResult.description.pricing);

      boolean hasDeal = mResult.isHotel &&
                        mFilteredHotelIds.contains(BookingFilter.TYPE_DEALS,
                                                   mResult.description.featureId);
      UiUtils.showIf(hasDeal, mSale);
    }

    private boolean isClosedVisible()
    {
      boolean isClosed = mResult.description.openNow == SearchResult.OPEN_NOW_NO;
      if (!isClosed)
        return false;

      boolean isNotPopular = mResult.getPopularity().getType() == Popularity.Type.NOT_POPULAR;

      return isNotPopular || !mResult.description.hasPopularityHigherPriority;
    }

    private boolean isPopularVisible()
    {
      boolean isNotPopular = mResult.getPopularity().getType() == Popularity.Type.NOT_POPULAR;
      if (isNotPopular)
        return false;

      boolean isClosed = mResult.description.openNow == SearchResult.OPEN_NOW_NO;

      return !isClosed || mResult.description.hasPopularityHigherPriority;
    }

    private void setBackground()
    {
      Context context = mSearchFragment.getActivity();
      @AttrRes
      int itemBg = ThemeUtils.getResource(context, R.attr.clickableBackground);
      int bottomPad = mFrame.getPaddingBottom();
      int topPad = mFrame.getPaddingTop();
      int rightPad = mFrame.getPaddingRight();
      int leftPad = mFrame.getPaddingLeft();
      mFrame.setBackgroundResource(needSpecificBackground() ? getSpecificBackground() : itemBg);
      // On old Android (4.1) after setting the view background the previous paddings
      // are discarded for unknown reasons, that's why restoring the previous paddings is needed.
      mFrame.setPadding(leftPad, topPad, rightPad, bottomPad);
    }

    @DrawableRes
    int getSpecificBackground()
    {
      return R.color.bg_search_available_hotel;
    }

    boolean needSpecificBackground()
    {
      return mResult.isHotel &&
             mFilteredHotelIds.contains(BookingFilter.TYPE_AVAILABILITY,
                                        mResult.description.featureId);
    }

    @Override
    void processClick(SearchResult result, int order)
    {
      mSearchFragment.showSingleResultOnMap(result, order);
    }
  }

  private class LocalAdsCustomerViewHolder extends ResultViewHolder
  {
    LocalAdsCustomerViewHolder(@NonNull View view)
    {
      super(view);
    }

    @Override
    boolean needSpecificBackground()
    {
      return true;
    }

    @Override
    @DrawableRes
    int getSpecificBackground()
    {
      @DrawableRes
      int resId = ThemeUtils.isNightTheme() ? R.drawable.search_la_customer_result_night
                                            : R.drawable.search_la_customer_result;
      return resId;
    }
  }

  SearchAdapter(SearchFragment fragment)
  {
    mSearchFragment = fragment;
    mClosedMarkerBackground = fragment.getResources().getDrawable(ThemeUtils.isNightTheme() ? R.drawable.search_closed_marker_night
                                                                                            : R.drawable.search_closed_marker);
  }

  @Override
  public SearchDataViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());

    switch (viewType)
    {
      case SearchResultTypes.TYPE_SUGGEST:
        return new SuggestViewHolder(inflater.inflate(R.layout.item_search_suggest, parent, false));

      case SearchResultTypes.TYPE_RESULT:
        return new ResultViewHolder(inflater.inflate(R.layout.item_search_result, parent, false));

      case SearchResultTypes.TYPE_LOCAL_ADS_CUSTOMER:
        return new LocalAdsCustomerViewHolder(inflater.inflate(R.layout.item_search_result, parent, false));

      case SearchResultTypes.TYPE_GOOGLE_ADS:
        return new GoogleAdsViewHolder(new FrameLayout(parent.getContext()));

      default:
        throw new IllegalArgumentException("Unhandled view type given");
    }
  }

  @Override
  public void onBindViewHolder(@NonNull SearchDataViewHolder holder, int position)
  {
    holder.bind(mResults[position], position);
  }

  @Override
  public int getItemViewType(int position)
  {
    return mResults[position].getItemViewType();
  }

  boolean showPopulateButton()
  {
    return (!RoutingController.get().isWaitingPoiPick() &&
            mResults != null &&
            mResults.length > 0 &&
            SearchResult.class.isInstance(mResults[0]) &&
            ((SearchResult) mResults[0]).type != SearchResultTypes.TYPE_SUGGEST);
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

  void refreshData(SearchData[] results)
  {
    mResults = results;
    notifyDataSetChanged();
  }

  void setFilteredHotels(@BookingFilter.Type int type, @NonNull FeatureId[] hotelsId)
  {
    mFilteredHotelIds.put(type, hotelsId);
    notifyDataSetChanged();
  }
}
