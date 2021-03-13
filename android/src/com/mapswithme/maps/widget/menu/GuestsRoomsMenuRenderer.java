package com.mapswithme.maps.widget.menu;

import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.search.FilterUtils;
import com.mapswithme.maps.widget.InteractiveCounterView;

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
  }

  @Override
  public void destroy()
  {
    mRoomsView.setChangeListener(null);
    mAdultsView.setChangeListener(null);
    mChildrenView.setChangeListener(null);
    mInfantsView.setChangeListener(null);
  }
}
