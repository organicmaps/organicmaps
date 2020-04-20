package com.mapswithme.maps.widget.menu;

import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.maps.maplayer.BottomSheetItem;
import com.mapswithme.maps.maplayer.LayersUtils;
import com.mapswithme.maps.maplayer.LayersAdapter;
import com.mapswithme.maps.widget.recycler.SpanningLinearLayoutManager;

import java.util.Objects;

public class MainMenuRenderer implements MenuRenderer
{
  @NonNull
  private final MainMenuOptionListener mListener;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView mLayersRecycler;
  @NonNull
  @SuppressWarnings("NullableProblems")
  private LayersAdapter mLayersAdapter;

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
    initLayersRecycler(view);
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

  private void initLayersRecycler(@NonNull View view)
  {
    mLayersRecycler = view.findViewById(R.id.layers_recycler);
    RecyclerView.LayoutManager layoutManager = new SpanningLinearLayoutManager(mLayersRecycler.getContext(),
                                                                               LinearLayoutManager.HORIZONTAL,
                                                                               false);
    mLayersRecycler.setLayoutManager(layoutManager);
    mLayersAdapter = new LayersAdapter(LayersUtils.createItems(mLayersRecycler.getContext(),
                                                               new SubwayItemClickListener(),
                                                               new TrafficItemClickListener(),
                                                               new IsolinesItemClickListener()));
    mLayersRecycler.setAdapter(mLayersAdapter);
  }

  @Override
  public void destroy()
  {
    // TODO: Implement.
  }

  private abstract class DefaultClickListener implements OnItemClickListener<BottomSheetItem>
  {
    @Override
    public final void onItemClick(@NonNull View v, @NonNull BottomSheetItem item)
    {
      item.getMode().toggle(v.getContext());
      onItemClickInternal(v, item);
      mLayersAdapter.notifyDataSetChanged();
    }

    abstract void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item);
  }

  private class SubwayItemClickListener extends DefaultClickListener
  {
    @Override
    void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      mListener.onSubwayLayerOptionSelected();
    }
  }

  private class TrafficItemClickListener extends DefaultClickListener
  {
    @Override
    void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      mListener.onTrafficLayerOptionSelected();
    }
  }

  private class IsolinesItemClickListener extends DefaultClickListener
  {
    @Override
    void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      mListener.onIsolinesLayerOptionSelected();
    }
  }
}
