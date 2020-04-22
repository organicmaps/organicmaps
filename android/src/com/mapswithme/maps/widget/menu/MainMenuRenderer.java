package com.mapswithme.maps.widget.menu;

import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.core.widget.NestedScrollView;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.UpdateInfo;
import com.mapswithme.maps.maplayer.BottomSheetItem;
import com.mapswithme.maps.maplayer.DefaultClickListener;
import com.mapswithme.maps.maplayer.LayersAdapter;
import com.mapswithme.maps.maplayer.LayersUtils;
import com.mapswithme.maps.widget.recycler.SpanningLinearLayoutManager;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

import java.util.Objects;

public class MainMenuRenderer implements MenuRenderer
{
  @NonNull
  private final MainMenuOptionListener mListener;
  @NonNull
  @SuppressWarnings("NullableProblems")
  private LayersAdapter mLayersAdapter;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mDownloadMapsCounter;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private NestedScrollView mScrollView;

  MainMenuRenderer(@NonNull MainMenuOptionListener listener)
  {
    mListener = listener;
  }

  @Override
  public void render()
  {
    renderDownloadMapsCounter();
  }

  private void renderDownloadMapsCounter()
  {
    UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    int count = info == null ? 0 : info.filesCount;
    UiUtils.showIf(count > 0, mDownloadMapsCounter);
    if (count > 0)
      mDownloadMapsCounter.setText(String.valueOf(count));
  }

  @Override
  public void initialize(@Nullable View view)
  {
    Objects.requireNonNull(view);
    mScrollView = (NestedScrollView) view;
    initLayersRecycler(view);
    TextView addPlace = view.findViewById(R.id.add_place);
    addPlace.setOnClickListener(v -> mListener.onAddPlaceOptionSelected());
    Graphics.tint(addPlace);
    TextView downloadGuides = view.findViewById(R.id.download_guides);
    downloadGuides.setOnClickListener(v -> mListener.onSearchGuidesOptionSelected());
    Graphics.tint(downloadGuides);
    TextView hotelSearch = view.findViewById(R.id.hotel_search);
    hotelSearch.setOnClickListener(v -> mListener.onHotelSearchOptionSelected());
    Graphics.tint(hotelSearch);
    View downloadMapsContainer = view.findViewById(R.id.download_maps_container);
    downloadMapsContainer.setOnClickListener(v -> mListener.onDownloadMapsOptionSelected());
    TextView downloadMaps = downloadMapsContainer.findViewById(R.id.download_maps);
    Graphics.tint(downloadMaps);
    mDownloadMapsCounter = downloadMapsContainer.findViewById(R.id.counter);
    TextView settings = view.findViewById(R.id.settings);
    settings.setOnClickListener(v -> mListener.onSettingsOptionSelected());
    Graphics.tint(settings);
    TextView share = view.findViewById(R.id.share);
    share.setOnClickListener(v -> mListener.onShareLocationOptionSelected());
    Graphics.tint(share);
  }

  private void initLayersRecycler(@NonNull View view)
  {
    RecyclerView layersRecycler = view.findViewById(R.id.layers_recycler);
    RecyclerView.LayoutManager layoutManager = new SpanningLinearLayoutManager(layersRecycler.getContext(),
                                                                               LinearLayoutManager.HORIZONTAL,
                                                                               false);
    layersRecycler.setLayoutManager(layoutManager);
    mLayersAdapter = new LayersAdapter();
    mLayersAdapter.setLayerModes(LayersUtils.createItems(layersRecycler.getContext(),
                                                         new SubwayItemClickListener(),
                                                         new TrafficItemClickListener(),
                                                         new IsolinesItemClickListener()));
    layersRecycler.setAdapter(mLayersAdapter);
  }

  @Override
  public void destroy()
  {
    // Do nothing by default.
  }

  @Override
  public void onHide()
  {
    mScrollView.scrollTo(0, 0);
  }

  // TODO: use this method to show layer toast.
  private static void showToast(@NonNull Context context, @StringRes int stringId)
  {
    Toast toast = Toast.makeText(context, stringId, Toast.LENGTH_LONG);
    toast.setGravity(Gravity.TOP, 0, 0);
    toast.show();
  }

  private class SubwayItemClickListener extends DefaultClickListener
  {
    SubwayItemClickListener()
    {
      super(mLayersAdapter);
    }

    @Override
    public void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      mListener.onSubwayLayerOptionSelected();
    }
  }

  private class TrafficItemClickListener extends DefaultClickListener
  {
    TrafficItemClickListener()
    {
      super(mLayersAdapter);
    }

    @Override
    public void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      mListener.onTrafficLayerOptionSelected();
    }
  }

  private class IsolinesItemClickListener extends DefaultClickListener
  {
    IsolinesItemClickListener()
    {
      super(mLayersAdapter);
    }

    @Override
    public void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      mListener.onIsolinesLayerOptionSelected();
    }
  }
}
