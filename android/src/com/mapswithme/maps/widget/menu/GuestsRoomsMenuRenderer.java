package com.mapswithme.maps.widget.menu;

import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.search.FilterUtils;
import com.mapswithme.maps.widget.InteractiveCounterView;
import com.mapswithme.util.statistics.Statistics;

import java.util.Objects;

public class GuestsRoomsMenuRenderer implements MenuRenderer
{
  @NonNull
  private final MenuRoomsGuestsListener mListener;
  @NonNull
  private final FilterUtils.RoomsGuestsCountProvider mProvider;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private InteractiveCounterView mRoomsView;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private InteractiveCounterView mAdultsView;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private InteractiveCounterView mChildrenView;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private InteractiveCounterView mInfantsView;

  public GuestsRoomsMenuRenderer(@NonNull MenuRoomsGuestsListener listener,
                                 @NonNull FilterUtils.RoomsGuestsCountProvider provider)
  {
    mListener = listener;
    mProvider = provider;
  }

  @Override
  public void render()
  {
    FilterUtils.RoomGuestCounts counts = mProvider.getRoomGuestCount();
    if (counts == null)
      return;

    mRoomsView.setCurrentValue(counts.getRooms());
    mAdultsView.setCurrentValue(counts.getAdults());
    mChildrenView.setCurrentValue(counts.getChildren());
    mInfantsView.setCurrentValue(counts.getInfants());
  }

  @Override
  public void onHide()
  {
    FilterUtils.RoomGuestCounts counts
        = new FilterUtils.RoomGuestCounts(mRoomsView.getCurrentValue(),
                                          mAdultsView.getCurrentValue(),
                                          mChildrenView.getCurrentValue(),
                                          mInfantsView.getCurrentValue());
    mListener.onRoomsGuestsApplied(counts);
  }

  @Override
  public void initialize(@Nullable View view)
  {
    Objects.requireNonNull(view);
    mRoomsView = view.findViewById(R.id.rooms);
    mRoomsView.setChangeListener(() -> onChangeCounterValue(Statistics.EventParam.ROOMS,
                                                            mRoomsView.getCurrentValue()));
    mAdultsView = view.findViewById(R.id.adults);
    mAdultsView.setChangeListener(() -> onChangeCounterValue(Statistics.EventParam.ADULTS,
                                                             mAdultsView.getCurrentValue()));
    mChildrenView = view.findViewById(R.id.children);
    mChildrenView.setChangeListener(() -> onChangeCounterValue(Statistics.EventParam.CHILDREN,
                                                               mChildrenView.getCurrentValue()));
    mInfantsView = view.findViewById(R.id.infants);
    mInfantsView.setChangeListener(() -> onChangeCounterValue(Statistics.EventParam.INFANTS,
                                                              mInfantsView.getCurrentValue()));
  }

  @Override
  public void destroy()
  {
    mRoomsView.setChangeListener(null);
    mAdultsView.setChangeListener(null);
    mChildrenView.setChangeListener(null);
    mInfantsView.setChangeListener(null);
  }

  private void onChangeCounterValue(@NonNull String name, int count)
  {
    Statistics.INSTANCE.trackQuickFilterClick(Statistics.EventParam.HOTEL, name, count);
  }
}
