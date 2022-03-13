package com.mapswithme.maps.maplayer;

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
import com.mapswithme.maps.widget.recycler.SpanningLinearLayoutManager;

public class ToggleMapLayerFragment extends Fragment
{
  @NonNull
  private final LayerItemClickListener mIsolinesListener;
  @NonNull
  private final LayerItemClickListener mSubwayListener;
  @NonNull
  private LayersAdapter mAdapter;

  public ToggleMapLayerFragment(@NonNull LayerItemClickListener isolinesListener, @NonNull LayerItemClickListener subwayListener)
  {
    this.mIsolinesListener = isolinesListener;
    this.mSubwayListener = subwayListener;
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View mRoot = inflater.inflate(R.layout.fragment_toggle_map_layer, container, false);
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
    mAdapter = new LayersAdapter();
    mAdapter.setLayerModes(LayersUtils.createItems(requireContext(),
                                                   new SubwayItemClickListener(),
                                                   new TrafficItemClickListener(),
                                                   new IsolinesItemClickListener()));
    recycler.setAdapter(mAdapter);
    recycler.setNestedScrollingEnabled(false);
  }

  public interface LayerItemClickListener
  {
    void onClick();
  }

  private class SubwayItemClickListener extends DefaultClickListener
  {
    private SubwayItemClickListener()
    {
      super(mAdapter);
    }

    @Override
    public void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      mSubwayListener.onClick();
    }
  }

  private class TrafficItemClickListener extends DefaultClickListener
  {
    private TrafficItemClickListener()
    {
      super(mAdapter);
    }

    @Override
    public void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {}
  }

  private class IsolinesItemClickListener extends AbstractIsoLinesClickListener
  {
    private IsolinesItemClickListener()
    {
      super(mAdapter);
    }

    @Override
    public void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      super.onItemClickInternal(v, item);
      mIsolinesListener.onClick();
    }
  }
}
