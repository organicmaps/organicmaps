package com.mapswithme.maps.maplayer;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.maplayer.isolines.IsolinesManager;
import com.mapswithme.maps.widget.recycler.SpanningLinearLayoutManager;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.Utils;

import java.util.ArrayList;
import java.util.List;

public class ToggleMapLayerFragment extends Fragment
{
  @Nullable
  private LayerItemClickListener mLayerItemClickListener;
  @Nullable
  private LayersAdapter mAdapter;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View mRoot = inflater.inflate(R.layout.fragment_toggle_map_layer, container, false);

    if (requireActivity() instanceof LayerItemClickListener)
      mLayerItemClickListener = ((LayerItemClickListener) requireActivity());

    initRecycler(mRoot);
    return mRoot;
  }

  private void initRecycler(@NonNull View root)
  {
    RecyclerView recycler = root.findViewById(R.id.recycler);
    RecyclerView.LayoutManager layoutManager = new SpanningLinearLayoutManager(requireContext(),
        LinearLayoutManager.HORIZONTAL,
        false);
    recycler.setLayoutManager(layoutManager);
    mAdapter = new LayersAdapter(getLayersItems());
    recycler.setAdapter(mAdapter);
    recycler.setNestedScrollingEnabled(false);
  }

  private List<LayerBottomSheetItem> getLayersItems()
  {
    List<Mode> availableLayers = LayersUtils.getAvailableLayers();
    List<LayerBottomSheetItem> items = new ArrayList<>();
    for (Mode layer : availableLayers)
    {
      items.add(LayerBottomSheetItem.create(requireContext(), layer, this::onItemClick));
    }
    return items;
  }

  private void onItemClick(@NonNull View v, @NonNull LayerBottomSheetItem item)
  {
    Mode mode = item.getMode();
    Context context = v.getContext();
    SharedPropertiesUtils.setLayerMarkerShownForLayerMode(context, mode);
    mAdapter.notifyDataSetChanged();
    if (IsolinesManager.from(context).shouldShowNotification())
      Utils.showSnackbar(context, v.getRootView(), R.string.isolines_toast_zooms_1_10);
    if (mLayerItemClickListener != null)
      mLayerItemClickListener.onLayerItemClick(mode);
  }

  public interface LayerItemClickListener
  {
    void onLayerItemClick(@NonNull Mode mode);
  }
}
