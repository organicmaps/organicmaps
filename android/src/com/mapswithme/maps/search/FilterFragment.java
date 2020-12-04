package com.mapswithme.maps.search;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.base.CustomNavigateUpListener;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.maps.widget.recycler.TagItemDecoration;
import com.mapswithme.maps.widget.recycler.TagLayoutManager;
import com.mapswithme.util.statistics.Statistics;

import java.util.HashSet;
import java.util.Objects;
import java.util.Set;

import static com.mapswithme.maps.search.FilterUtils.combineFilters;
import static com.mapswithme.maps.search.FilterUtils.findPriceFilter;
import static com.mapswithme.maps.search.FilterUtils.findRatingFilter;
import static com.mapswithme.maps.search.FilterUtils.findTypeFilter;
import static com.mapswithme.maps.search.FilterUtils.makeOneOf;

public class FilterFragment extends BaseMwmToolbarFragment
    implements HotelsTypeAdapter.OnTypeSelectedListener
{
  static final String ARG_FILTER = "arg_filter";
  static final String ARG_FILTER_PARAMS = "arg_filter_params";
  @Nullable
  private CustomNavigateUpListener mNavigateUpListener;
  @Nullable
  private Listener mListener;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private RatingFilterView mRating;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private PriceFilterView mPrice;
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

  @NonNull
  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(root, requireActivity())
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
    // Explicit casting is needed in this case, otherwise the crash is obtained in runtime,
    // seems like a bug in current compiler (Java 8)
    mPrice = root.<PriceFilterView>findViewById(R.id.price);
    View content = root.findViewById(R.id.content);
    RecyclerView type = content.findViewById(R.id.type);
    type.setLayoutManager(new TagLayoutManager());
    type.setNestedScrollingEnabled(false);
    int tagsDecorator = R.drawable.divider_transparent_half;
    Drawable drawable = ContextCompat.getDrawable(requireContext(), tagsDecorator);
    type.addItemDecoration(new TagItemDecoration(Objects.requireNonNull(drawable)));
    mTypeAdapter = new HotelsTypeAdapter(this);
    type.setAdapter(mTypeAdapter);
    root.findViewById(R.id.done).setOnClickListener(v -> onFilterClicked());

    Bundle args = getArguments();
    HotelsFilter filter = null;
    BookingFilterParams params = null;
    if (args != null)
    {
      filter = args.getParcelable(ARG_FILTER);
      params = args.getParcelable(ARG_FILTER_PARAMS);
    }
    updateViews(filter, params);
    return root;
  }

  private void onFilterClicked()
  {
    if (mListener == null)
      return;

    HotelsFilter filter = populateFilter();
    mListener.onFilterApply(filter);
    BookingFilterParams params = null;
    if (getArguments() != null)
      params = getArguments().getParcelable(ARG_FILTER_PARAMS);
    Statistics.INSTANCE.trackFilterApplyEvent(FilterUtils.toAppliedFiltersString(filter, params));
    UserActionsLogger.logBookingFilterUsedEvent();
  }

  @Override
  public void onActivityCreated(@Nullable Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);
    Statistics.INSTANCE.trackFilterOpenEvent();
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.booking_filters);
    getToolbarController().getToolbar().findViewById(R.id.reset)
        .setOnClickListener(
            v ->
            {
              Statistics.INSTANCE.trackFilterEvent(Statistics.EventName.SEARCH_FILTER_RESET,
                                                   Statistics.EventParam.HOTEL);
              updateViews(null, null);
            });
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

  private void updateViews(@Nullable HotelsFilter filter, @Nullable BookingFilterParams params)
  {
    if (filter == null)
    {
      mRating.update(null);
      mPrice.update(null);
      if (mTypeAdapter != null)
        updateTypeAdapter(mTypeAdapter, null);
      return;
    }

    mRating.update(findRatingFilter(filter));
    mPrice.update(findPriceFilter(filter));
    if (mTypeAdapter != null)
      updateTypeAdapter(mTypeAdapter, findTypeFilter(filter));
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
    {
      Statistics.INSTANCE.trackFilterClick(Statistics.EventParam.HOTEL,
                                           new Pair<>(Statistics.EventParam.TYPE, type.getTag()));
      mHotelTypes.add(type);
    }
    else
    {
      mHotelTypes.remove(type);
    }
  }

  interface Listener
  {
    void onFilterApply(@Nullable HotelsFilter filter);
  }
}
