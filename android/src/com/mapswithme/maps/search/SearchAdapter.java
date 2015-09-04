package com.mapswithme.maps.search;

import android.graphics.Typeface;
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
import com.mapswithme.util.UiUtils;

class SearchAdapter extends RecyclerView.Adapter<SearchAdapter.BaseViewHolder>
{
  private static final int TYPE_POPULATE_BUTTON = 0;
  private static final int TYPE_SUGGEST = 1;
  private static final int TYPE_RESULT = 2;

  private static final int NO_RESULTS = -1;

  protected static abstract class BaseViewHolder extends RecyclerView.ViewHolder
  {
    SearchResult mResult;
    // Position within search results
    int mOrder;

    BaseViewHolder(View view)
    {
      super(view);
    }

    void bind(@NonNull SearchResult result, int order)
    {
      mResult = result;
      mOrder = order;
    }
  }

  private class PopulateResultsViewHolder extends BaseViewHolder
  {
    PopulateResultsViewHolder(View view)
    {
      super(view);
      view.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          mSearchFragment.showAllResultsOnMap();
        }
      });
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
      return (TextView)itemView;
    }

    @Override
    void processClick(SearchResult result, int order)
    {
      mSearchFragment.setSearchQuery(result.suggestion);
    }
  }

  private class ResultViewHolder extends BaseResultViewHolder
  {
    final TextView mName;
    final View mClosedMarker;
    final TextView mDescription;
    final TextView mRegion;
    final TextView mDistance;


    // FIXME: Better format based on result type
    private CharSequence formatDescription(SearchResult result)
    {
      final SpannableStringBuilder res = new SpannableStringBuilder(result.description.featureType);
      final SpannableStringBuilder tail = new SpannableStringBuilder();

      final int stars = Math.min(result.description.stars, 5);
      if (stars > 0)
      {
        // Colorize last dimmed stars: "★ ★ ★ ★ ★"
        final SpannableStringBuilder sb = new SpannableStringBuilder("\u2605 \u2605 \u2605 \u2605 \u2605");
        if (stars < 5)
        {
          final int start = sb.length() - ((5 - stars) * 2 - 1);
          sb.setSpan(new ForegroundColorSpan(itemView.getResources().getColor(R.color.search_star_dimmed)),
                     start, sb.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
        }

        tail.append(sb);
      }
      else if (!TextUtils.isEmpty(result.description.cuisine))
        tail.append(result.description.cuisine);

      if (!TextUtils.isEmpty(tail))
        res.append(" \u2022 ")
           .append(tail);

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

      UiUtils.showIf(result.description.closedNow, mClosedMarker);
      UiUtils.setTextAndHideIfEmpty(mDescription, formatDescription(result));
      UiUtils.setTextAndHideIfEmpty(mRegion, result.description.region);
      UiUtils.setTextAndHideIfEmpty(mDistance, result.description.distance);
    }

    @Override
    void processClick(SearchResult result, int order)
    {
      mSearchFragment.showSingleResultOnMap(order);
      notifyDataSetChanged();
    }
  }

  private final SearchFragment mSearchFragment;
  private int mResultsCount = NO_RESULTS;
  private int mQueryId;

  public SearchAdapter(SearchFragment fragment)
  {
    mSearchFragment = fragment;
  }

  @Override
  public BaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());

    switch (viewType)
    {
      case TYPE_POPULATE_BUTTON:
        return new PopulateResultsViewHolder(inflater.inflate(R.layout.item_search_populate, parent, false));

      case TYPE_SUGGEST:
        return new SuggestViewHolder(inflater.inflate(R.layout.item_search_suggest, parent, false));

      case TYPE_RESULT:
        return new ResultViewHolder(inflater.inflate(R.layout.item_search_result, parent, false));

      default:
        throw new IllegalArgumentException("Unhandled view type given");
    }
  }

  @Override
  public void onBindViewHolder(BaseViewHolder holder, int position)
  {
    if (showPopulateButton())
    {
      if (position == 0)
        return;

      position--;
    }

    final SearchResult result = mSearchFragment.getResult(position, mQueryId);
    if (result != null)
      holder.bind(result, position);
  }

  @Override
  public int getItemViewType(int position)
  {
    if (showPopulateButton())
    {
      if (position == 0)
        return TYPE_POPULATE_BUTTON;

      position--;
    }

    final SearchResult result = mSearchFragment.getResult(position, mQueryId);
    switch (result.type)
    {
      case SearchResult.TYPE_SUGGEST:
        return TYPE_SUGGEST;

      case SearchResult.TYPE_RESULT:
        return TYPE_RESULT;

      default:
        throw new IllegalArgumentException("Unhandled SearchResult type");
    }
  }

  boolean showPopulateButton()
  {
    return (mResultsCount > 0 && !mSearchFragment.isSearchRunning());
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public int getItemCount()
  {
    if (mResultsCount == NO_RESULTS)
      return 0;

    if (showPopulateButton())
      return mResultsCount + 1;

    return mResultsCount;
  }

  public void clear()
  {
    refreshData(0, 0);
  }

  public void refreshData(int count, int queryId)
  {
    mResultsCount = count;
    mQueryId = queryId;

    notifyDataSetChanged();
  }
}