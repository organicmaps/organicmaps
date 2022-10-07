package com.mapswithme.maps.search;

import android.content.Context;
import android.content.res.Resources;
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
      view.setOnClickListener(v -> processClick(mResult, mOrder));
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
    final TextView mOpen;
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
        tail.append(" • ").append(result.description.airportIata);
      }
      else if (!TextUtils.isEmpty(result.description.roadShields))
      {
        tail.append(" • ").append(result.description.roadShields);
      }
      else
      {
        if (!TextUtils.isEmpty(result.description.brand))
        {
          tail.append(" • ").append(Utils.getLocalizedBrand(mFrame.getContext(), result.description.brand));
        }
        if (!TextUtils.isEmpty(result.description.cuisine))
        {
          tail.append(" • ").append(result.description.cuisine);
        }
      }

      if (result.isHotel && result.stars != 0)
      {
        tail.append(" • ").append("★★★★★★★".substring(0, Math.min(7, result.stars)));
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
      mOpen = view.findViewById(R.id.open);
      mDescription =  view.findViewById(R.id.description);
      mRegion = view.findViewById(R.id.region);
      mDistance = view.findViewById(R.id.distance);
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

      formatOpeningHours(mResult);
      UiUtils.setTextAndHideIfEmpty(mDescription, formatDescription(mResult));
      UiUtils.setTextAndHideIfEmpty(mRegion, mResult.description.region);
      UiUtils.setTextAndHideIfEmpty(mDistance, mResult.description.distance);
    }

    private void formatOpeningHours(SearchResult result)
    {
      final Resources resources = mSearchFragment.getResources();

      switch (result.description.openNow)
      {
        case SearchResult.OPEN_NOW_YES:
          if (result.description.minutesUntilClosed < 60)   // less than 1 hour
          {
            final String time = result.description.minutesUntilClosed + " " +
                          resources.getString(R.string.minute);
            final String string = resources.getString(R.string.closes_in, time);

            UiUtils.setTextAndShow(mOpen, string);
            mOpen.setTextColor(resources.getColor(R.color.base_yellow));
          }
          else
          {
            UiUtils.setTextAndShow(mOpen, resources.getString(R.string.editor_time_open));
            mOpen.setTextColor(resources.getColor(R.color.base_green));
          }
          break;

        case SearchResult.OPEN_NOW_NO:
          if (result.description.minutesUntilOpen < 60) // less than 1 hour
          {
            final String time = result.description.minutesUntilOpen + " " +
                          resources.getString(R.string.minute);
            final String string = resources.getString(R.string.opens_in, time);

            UiUtils.setTextAndShow(mOpen, string);
            mOpen.setTextColor(resources.getColor(R.color.base_red));
          }
          else
          {
            UiUtils.setTextAndShow(mOpen, resources.getString(R.string.closed));
            mOpen.setTextColor(resources.getColor(R.color.base_red));
          }
          break;

        default:
          UiUtils.hide(mOpen);
          break;
      }
    }

    private void setBackground()
    {
      final Context context = mSearchFragment.requireActivity();
      final int itemBg = ThemeUtils.getResource(context, R.attr.clickableBackground);
      mFrame.setBackgroundResource(itemBg);
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
