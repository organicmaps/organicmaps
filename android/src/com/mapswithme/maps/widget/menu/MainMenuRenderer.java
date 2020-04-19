package com.mapswithme.maps.widget.menu;

import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;

import java.util.Objects;

public class MainMenuRenderer implements MenuRenderer
{
  @NonNull
  private final MainMenuOptionListener mListener;

  MainMenuRenderer(@NonNull MainMenuOptionListener listener)
  {
    mListener = listener;
  }

  @Override
  public void render()
  {
    // TODO: Implement.
  }

  @Override
  public void initialize(@Nullable View view)
  {
    Objects.requireNonNull(view);
    View addPlace = view.findViewById(R.id.add_place);
    addPlace.setOnClickListener(v -> mListener.onAddPlaceOptionSelected());
    View downloadGuides = view.findViewById(R.id.download_guides);
    downloadGuides.setOnClickListener(v -> mListener.onSearchGuidesOptionSelected());
    View hotelSearch = view.findViewById(R.id.hotel_search);
    hotelSearch.setOnClickListener(v -> mListener.onHotelSearchOptionSelected());
    View downloadMaps = view.findViewById(R.id.download_maps_container);
    downloadMaps.setOnClickListener(v -> mListener.onDownloadMapsOptionSelected());
    View settings = view.findViewById(R.id.settings);
    settings.setOnClickListener(v -> mListener.onSettingsOptionSelected());
    View share = view.findViewById(R.id.share);
    share.setOnClickListener(v -> mListener.onShareLocationOptionSelected());
  }

  @Override
  public void destroy()
  {
    // TODO: Implement.
  }
}
