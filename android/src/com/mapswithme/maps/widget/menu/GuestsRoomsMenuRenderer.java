package com.mapswithme.maps.widget.menu;

import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.search.FilterUtils;
import com.mapswithme.maps.widget.InteractiveCounterView;

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
    mAdultsView = view.findViewById(R.id.adults);
    mChildrenView = view.findViewById(R.id.children);
    mInfantsView = view.findViewById(R.id.infants);
  }

  @Override
  public void destroy()
  {
    // No op.
  }
}
