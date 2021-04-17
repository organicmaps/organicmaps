package com.mapswithme.maps.search;

import android.content.Context;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.AttrRes;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import static com.mapswithme.maps.search.SearchResult.TYPE_RESULT;
import static com.mapswithme.maps.search.SearchResult.TYPE_SUGGEST;

class SearchAdapter extends RecyclerView.Adapter<SearchAdapter.SearchDataViewHolder>
{
  private final SearchFragment mSearchFragment;
  @Nullable
  private SearchResult[] mResults;
  @NonNull
  private final Drawable mClosedMarkerBackground;

  static abstract class SearchDataViewHolder extends RecyclerView.ViewHolder
  {
    SearchDataViewHolder(@NonNull View itemView)
    {
      super(itemView);
    }

    abstract void bind(@NonNull SearchResult result, int position);
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
    void bind(@NonNull SearchResult result, int order)
    {
      mResult = result;
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

  private class ResultViewHolder extends BaseResultViewHolder
  {
    @NonNull
    final View mFrame;
    @NonNull
    final TextView mName;
    @NonNull
    final View mClosedMarker;
    @NonNull
    final TextView mDescription;
    @NonNull
    final TextView mRegion;
    @NonNull
    final TextView mDistance;

    @Override
    int getTintAttr()
    {
      return 0;
    }

    // FIXME: Better format based on result type
    private CharSequence formatDescription(SearchResult result)
    {
      String localizedType = Utils.getLocalizedFeatureType(mFrame.getContext(),
                                                           result.description.featureType);
      final SpannableStringBuilder res = new SpannableStringBuilder(localizedType);
      final SpannableStringBuilder tail = new SpannableStringBuilder();

      if (!TextUtils.isEmpty(result.description.airportIata))
      {
        tail.append(" • " + result.description.airportIata);
      }
      else if (!TextUtils.isEmpty(result.description.roadShields))
      {
        tail.append(" • " + result.description.roadShields);
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
      mDescription =  view.findViewById(R.id.description);
      mRegion = view.findViewById(R.id.region);
      mDistance = view.findViewById(R.id.distance);

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
      setBackground();
      // TODO: Support also "Open Now" mark.

      UiUtils.showIf(isClosedVisible(), mClosedMarker);
      UiUtils.setTextAndHideIfEmpty(mDescription, formatDescription(mResult));
      UiUtils.setTextAndHideIfEmpty(mRegion, mResult.description.region);
      UiUtils.setTextAndHideIfEmpty(mDistance, mResult.description.distance);
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
      int itemBg = ThemeUtils.getResource(context, R.attr.clickableBackground);
      int bottomPad = mFrame.getPaddingBottom();
      int topPad = mFrame.getPaddingTop();
      int rightPad = mFrame.getPaddingRight();
      int leftPad = mFrame.getPaddingLeft();
      mFrame.setBackgroundResource(itemBg);
      // On old Android (4.1) after setting the view background the previous paddings
      // are discarded for unknown reasons, that's why restoring the previous paddings is needed.
      mFrame.setPadding(leftPad, topPad, rightPad, bottomPad);
    }

    @Override
    void processClick(SearchResult result, int order)
    {
      mSearchFragment.showSingleResultOnMap(result, order);
    }
  }

  SearchAdapter(SearchFragment fragment)
  {
    mSearchFragment = fragment;
    mClosedMarkerBackground = fragment.getResources().getDrawable(
        ThemeUtils.isNightTheme(fragment.requireContext()) ?
        R.drawable.search_closed_marker_night :
        R.drawable.search_closed_marker);
  }

  @Override
  public SearchDataViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());

    switch (viewType)
    {
      case TYPE_SUGGEST:
        return new SuggestViewHolder(inflater.inflate(R.layout.item_search_suggest, parent, false));

      case TYPE_RESULT:
        return new ResultViewHolder(inflater.inflate(R.layout.item_search_result, parent, false));

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
    return mResults[position].type;
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

  void refreshData(@Nullable SearchResult[] results)
  {
    mResults = results;
    notifyDataSetChanged();
  }
}
