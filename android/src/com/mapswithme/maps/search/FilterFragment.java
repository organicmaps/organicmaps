package com.mapswithme.maps.search;

import android.app.DatePickerDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.Drawable;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.support.v7.widget.RecyclerView;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.maps.widget.recycler.TagItemDecoration;
import com.mapswithme.maps.widget.recycler.TagLayoutManager;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.DateUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

import java.text.DateFormat;
import java.util.Calendar;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.concurrent.TimeUnit;

public class FilterFragment extends BaseMwmToolbarFragment
    implements HotelsTypeAdapter.OnTypeSelectedListener
{
  static final String ARG_FILTER = "arg_filter";
  static final String ARG_FILTER_PARAMS = "arg_filter_params";
  private static final int MAX_STAYING_DAYS = 30;
  private static final int MAX_CHECKIN_WINDOW_IN_DAYS = 360;
  @NonNull
  private final DateFormat mDateFormatter = DateUtils.getMediumDateFormatter();
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
  private TextView mCheckIn;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mCheckOut;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mOfflineWarning;
  @NonNull
  private final Drawable mTagsDecorator
      = ContextCompat.getDrawable(MwmApplication.get(), R.drawable.divider_transparent_half);

  @NonNull
  private final Set<HotelsFilter.HotelType> mHotelTypes = new HashSet<>();
  @Nullable
  private HotelsTypeAdapter mTypeAdapter;
  @NonNull
  private Calendar mCheckinDate = Calendar.getInstance();
  @NonNull
  private Calendar mCheckoutDate = getDayAfter(mCheckinDate);
  @NonNull
  private final DatePickerDialog.OnDateSetListener mCheckinListener = (view, year, monthOfYear,
                                                                       dayOfMonth) ->
  {
    Calendar chosenDate = Calendar.getInstance();
    chosenDate.set(year, monthOfYear, dayOfMonth);
    mCheckinDate = chosenDate;
    if (mCheckinDate.after(mCheckoutDate))
    {
      mCheckoutDate =  getDayAfter(mCheckinDate);
      mCheckOut.setText(mDateFormatter.format(mCheckoutDate.getTime()));
    }
    else
    {
      long difference = mCheckoutDate.getTimeInMillis() - mCheckinDate.getTimeInMillis();
      int days = (int) TimeUnit.MILLISECONDS.toDays(difference);
      if (days > MAX_STAYING_DAYS)
      {
        mCheckoutDate = getMaxDateForCheckout(mCheckinDate);
        mCheckOut.setText(mDateFormatter.format(mCheckoutDate.getTime()));
      }
    }
    mCheckIn.setText(mDateFormatter.format(chosenDate.getTime()));
  };
  @NonNull
  private final DatePickerDialog.OnDateSetListener mCheckoutListener = (view, year, monthOfYear,
                                                                        dayOfMonth) ->
  {
    Calendar chosenDate = Calendar.getInstance();
    chosenDate.set(year, monthOfYear, dayOfMonth);
    mCheckoutDate = chosenDate;
    mCheckOut.setText(mDateFormatter.format(mCheckoutDate.getTime()));
  };
  @NonNull
  private final BroadcastReceiver mNetworkStateReceiver = new BroadcastReceiver()
  {
    @Override
    public void onReceive(Context context, Intent intent)
    {
      enableDateViewsIfConnected();
    }
  };

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
  public void onStart()
  {
    super.onStart();
    IntentFilter filter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
    getActivity().registerReceiver(mNetworkStateReceiver, filter);
  }

  @Override
  public void onStop()
  {
    getActivity().unregisterReceiver(mNetworkStateReceiver);
    super.onStop();
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
    initDateViews(root);
    mRating = root.findViewById(R.id.rating);
    // Explicit casting is needed in this case, otherwise the crash is obtained in runtime,
    // seems like a bug in current compiler (Java 8)
    mPrice = root.<PriceFilterView>findViewById(R.id.price);
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
          mListener.onFilterApply(filter, new BookingFilterParams(mCheckinDate.getTimeInMillis(),
                                                                  mCheckoutDate.getTimeInMillis(),
                                                                  BookingFilterParams.Room.DEFAULT));
          Statistics.INSTANCE.trackFilterEvent(Statistics.EventName.SEARCH_FILTER_APPLY,
                                               Statistics.EventParam.HOTEL);
        });

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

  @Override
  public void onActivityCreated(@Nullable Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);
    Statistics.INSTANCE.trackFilterEvent(Statistics.EventName.SEARCH_FILTER_OPEN,
                                         Statistics.EventParam.HOTEL);
  }

  private void initDateViews(View root)
  {
    mCheckIn = root.findViewById(R.id.checkIn);
    mCheckIn.setOnClickListener(
        v ->
        {
          DatePickerDialog dialog =
              new DatePickerDialog(getActivity(), mCheckinListener, mCheckinDate.get(Calendar.YEAR),
                                   mCheckinDate.get(Calendar.MONTH),
                                   mCheckinDate.get(Calendar.DAY_OF_MONTH));
          dialog.getDatePicker().setMinDate(getMinDateForCheckin().getTimeInMillis());
          dialog.getDatePicker().setMaxDate(getMaxDateForCheckin().getTimeInMillis());
          dialog.show();
          Statistics.INSTANCE.trackFilterClick(Statistics.EventParam.HOTEL,
                                               new Pair<>(Statistics.EventParam.DATE,
                                                          Statistics.ParamValue.CHECKIN));

        });
    mCheckOut = root.findViewById(R.id.checkOut);
    mCheckOut.setOnClickListener(
        v ->
        {
          DatePickerDialog dialog
              = new DatePickerDialog(getActivity(), mCheckoutListener,
                                     mCheckoutDate.get(Calendar.YEAR),
                                     mCheckoutDate.get(Calendar.MONTH),
                                     mCheckoutDate.get(Calendar.DAY_OF_MONTH));

          dialog.getDatePicker().setMinDate(getDayAfter(mCheckinDate).getTimeInMillis());
          dialog.getDatePicker().setMaxDate(getMaxDateForCheckout(mCheckinDate).getTimeInMillis());
          dialog.show();
          Statistics.INSTANCE.trackFilterClick(Statistics.EventParam.HOTEL,
                                               new Pair<>(Statistics.EventParam.DATE,
                                                          Statistics.ParamValue.CHECKOUT));
        });


    mOfflineWarning = root.findViewById(R.id.offlineWarning);
    enableDateViewsIfConnected();
  }

  private void enableDateViewsIfConnected()
  {
    UiUtils.showIf(!ConnectionState.isConnected(), mOfflineWarning);
    mCheckIn.setEnabled(ConnectionState.isConnected());
    mCheckOut.setEnabled(ConnectionState.isConnected());
  }

  @NonNull
  private static Calendar getMinDateForCheckin()
  {
    Calendar date = Calendar.getInstance();
    // This little subtraction is needed to avoid the crash on old androids (e.g. 4.4).
    date.add(Calendar.SECOND, -1);
    return date;
  }

  @NonNull
  private static Calendar getMaxDateForCheckin()
  {
    Calendar date = Calendar.getInstance();
    date.add(Calendar.DAY_OF_YEAR, MAX_CHECKIN_WINDOW_IN_DAYS);
    return date;
  }

  @NonNull
  private static Calendar getMaxDateForCheckout(@NonNull Calendar checkin)
  {
    long difference = checkin.getTimeInMillis() - System.currentTimeMillis();
    int daysToCheckin = (int) TimeUnit.MILLISECONDS.toDays(difference);
    int leftDays = MAX_CHECKIN_WINDOW_IN_DAYS - daysToCheckin;
    Calendar date = Calendar.getInstance();
    date.setTime(checkin.getTime());
    date.add(Calendar.DAY_OF_YEAR, leftDays >= MAX_STAYING_DAYS ? MAX_STAYING_DAYS : leftDays);
    return date;
  }

  @NonNull
  private static Calendar getDayAfter(@NonNull Calendar date)
  {
    Calendar dayAfter = Calendar.getInstance();
    dayAfter.setTime(date.getTime());
    dayAfter.add(Calendar.DAY_OF_YEAR, 1);
    return dayAfter;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController.setTitle(R.string.booking_filters);
    mToolbarController
        .findViewById(R.id.reset)
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

  private void updateViews(@Nullable HotelsFilter filter, @Nullable BookingFilterParams params)
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

    updateDateViews(params);
  }

  private void updateDateViews(@Nullable BookingFilterParams params)
  {
    if (params == null)
    {
      mCheckinDate = Calendar.getInstance();
      mCheckIn.setText(mDateFormatter.format(mCheckinDate.getTime()));
      mCheckoutDate = getDayAfter(mCheckinDate);
      mCheckOut.setText(mDateFormatter.format(mCheckoutDate.getTime()));
    }
    else
    {
      Calendar checkin = Calendar.getInstance();
      checkin.setTimeInMillis(params.getCheckinMillisec());
      mCheckinDate = checkin;
      mCheckIn.setText(mDateFormatter.format(mCheckinDate.getTime()));

      Calendar checkout = Calendar.getInstance();
      checkout.setTimeInMillis(params.getCheckoutMillisec());
      mCheckoutDate = checkout;
      mCheckOut.setText(mDateFormatter.format(mCheckoutDate.getTime()));
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
    void onFilterApply(@Nullable HotelsFilter filter, @Nullable BookingFilterParams params);
  }
}
