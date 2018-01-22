package com.mapswithme.maps.search;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.maps.widget.recycler.TagItemDecoration;
import com.mapswithme.maps.widget.recycler.TagLayoutManager;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

public class FilterFragment extends BaseMwmToolbarFragment
    implements HotelsTypeAdapter.OnTypeSelectedListener
{
  static final String ARG_FILTER = "arg_filter";
  @Nullable
  private CustomNavigateUpListener mNavigateUpListener;
  @Nullable
  private Listener mListener;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RatingFilterView mRating;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PriceFilterView mPrice;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private final Drawable mTagsDecorator
      = ContextCompat.getDrawable(MwmApplication.get(), R.drawable.divider_transparent_half);
  @NonNull
  private final Set<HotelsFilter.HotelType> mHotelTypes = new HashSet<>();
  @Nullable
  private HotelsTypeAdapter mTypeAdapter;

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);

    if (context instanceof CustomNavigateUpListener)
      mNavigateUpListener = (CustomNavigateUpListener) context;

    if (context instanceof Listener)
      mListener = (Listener) context;
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    mNavigateUpListener = null;
    mListener = null;
  }

  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(root, getActivity())
    {
      @Override
      public void onUpClick()
      {
        if (mNavigateUpListener == null)
          return;

        mNavigateUpListener.customOnNavigateUp();
      }
    };
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_search_filter, container, false);
    mRating = root.findViewById(R.id.rating);
    mPrice = root.findViewById(R.id.price);
    View content = root.findViewById(R.id.content);
    RecyclerView type = content.findViewById(R.id.type);
    type.setLayoutManager(new TagLayoutManager());
    type.setNestedScrollingEnabled(false);
    type.addItemDecoration(new TagItemDecoration(mTagsDecorator));
    mTypeAdapter = new HotelsTypeAdapter(this);
    type.setAdapter(mTypeAdapter);
    root.findViewById(R.id.done).setOnClickListener(
        v ->
        {
          if (mListener == null)
            return;

          HotelsFilter filter = populateFilter();
          mListener.onFilterApply(filter);
        });
    Bundle args = getArguments();
    HotelsFilter filter = null;
    if (args != null)
      filter = args.getParcelable(ARG_FILTER);
    updateViews(filter);
    return root;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController.setTitle(R.string.booking_filters);
    mToolbarController.findViewById(R.id.reset).setOnClickListener(v -> updateViews(null));
  }

  @Nullable
  private HotelsFilter populateFilter()
  {
    mPrice.updateFilter();
    final HotelsFilter.RatingFilter rating = mRating.getFilter();
    final HotelsFilter price = mPrice.getFilter();
    final HotelsFilter.OneOf oneOf = makeOneOf(mHotelTypes.iterator());

    return combineFilters(rating, price, oneOf);
  }

  @Nullable
  private HotelsFilter.OneOf makeOneOf(@NonNull Iterator<HotelsFilter.HotelType> iterator)
  {
    if (!iterator.hasNext())
      return null;

    HotelsFilter.HotelType type = iterator.next();
    return new HotelsFilter.OneOf(type, makeOneOf(iterator));
  }

  @Nullable
  private HotelsFilter combineFilters(@NonNull HotelsFilter... filters)
  {
    HotelsFilter result = null;
    for (HotelsFilter filter : filters)
    {
      if (result == null)
      {
        result = filter;
        continue;
      }

      if (filter != null)
        result = new HotelsFilter.And(filter, result);
    }

    return result;
  }

  private void updateViews(@Nullable HotelsFilter filter)
  {
    if (filter == null)
    {
      mRating.update(null);
      mPrice.update(null);
      if (mTypeAdapter != null)
        updateTypeAdapter(mTypeAdapter, null);
    }
    else
    {
      mRating.update(findRatingFilter(filter));
      mPrice.update(findPriceFilter(filter));
      if (mTypeAdapter != null)
        updateTypeAdapter(mTypeAdapter, findTypeFilter(filter));
    }
  }

  @Nullable
  private HotelsFilter.RatingFilter findRatingFilter(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.RatingFilter)
      return (HotelsFilter.RatingFilter) filter;

    HotelsFilter.RatingFilter result;
    if (filter instanceof HotelsFilter.And)
    {
      HotelsFilter.And and = (HotelsFilter.And) filter;
      result = findRatingFilter(and.mLhs);
      if (result == null)
        result = findRatingFilter(and.mRhs);

      return result;
    }

    return null;
  }

  @Nullable
  private HotelsFilter findPriceFilter(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.PriceRateFilter)
      return filter;

    if (filter instanceof HotelsFilter.Or)
    {
      HotelsFilter.Or or = (HotelsFilter.Or) filter;
      if (or.mLhs instanceof HotelsFilter.PriceRateFilter
          && or.mRhs instanceof HotelsFilter.PriceRateFilter )
      {
        return filter;
      }
    }

    HotelsFilter result;
    if (filter instanceof HotelsFilter.And)
    {
      HotelsFilter.And and = (HotelsFilter.And) filter;
      result = findPriceFilter(and.mLhs);
      if (result == null)
        result = findPriceFilter(and.mRhs);

      return result;
    }

    return null;
  }

  @Nullable
  private HotelsFilter.OneOf findTypeFilter(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.OneOf)
      return (HotelsFilter.OneOf) filter;

    HotelsFilter.OneOf result;
    if (filter instanceof HotelsFilter.And)
    {
      HotelsFilter.And and = (HotelsFilter.And) filter;
      result = findTypeFilter(and.mLhs);
      if (result == null)
        result = findTypeFilter(and.mRhs);

      return result;
    }

    return null;
  }

  private void updateTypeAdapter(@NonNull HotelsTypeAdapter typeAdapter,
                                 @Nullable HotelsFilter.OneOf types)
  {
    mHotelTypes.clear();
    if (types != null)
      populateHotelTypes(mHotelTypes, types);
    typeAdapter.updateItems(mHotelTypes);
  }

  private void populateHotelTypes(@NonNull Set<HotelsFilter.HotelType> hotelTypes,
                                  @NonNull HotelsFilter.OneOf types)
  {
    hotelTypes.add(types.mType);
    if (types.mTile != null)
      populateHotelTypes(hotelTypes, types.mTile);
  }

  @Override
  public void onTypeSelected(boolean selected, @NonNull HotelsFilter.HotelType type)
  {
    if (selected)
      mHotelTypes.add(type);
    else
      mHotelTypes.remove(type);
  }

  interface Listener
  {
    void onFilterApply(@Nullable HotelsFilter filter);
  }
}
