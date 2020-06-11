package com.mapswithme.maps.maplayer;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.mapswithme.maps.R;
import com.mapswithme.maps.maplayer.subway.OnSubwayLayerToggleListener;
import com.mapswithme.maps.maplayer.traffic.OnTrafficLayerToggleListener;
import com.mapswithme.maps.maplayer.guides.AbstractGuidesClickListener;
import com.mapswithme.maps.base.NoConnectionListener;
import com.mapswithme.maps.widget.recycler.SpanningLinearLayoutManager;

import java.util.Objects;

public class ToggleMapLayerDialog extends DialogFragment
{
  @NonNull
  @SuppressWarnings("NullableProblems")
  private LayersAdapter mAdapter;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mRoot;

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    BottomSheetDialog dialog = new BottomSheetDialog(requireActivity());
    LayoutInflater inflater = requireActivity().getLayoutInflater();
    mRoot = inflater.inflate(R.layout.fragment_toggle_map_layer, null, false);
    dialog.setOnShowListener(this::onShow);
    dialog.setContentView(mRoot);
    initChildren(mRoot);
    return dialog;
  }

  private void onShow(@NonNull DialogInterface dialogInterface)
  {
    BottomSheetDialog dialog = (BottomSheetDialog) dialogInterface;
    View bottomSheet = dialog.findViewById(com.google.android.material.R.id.design_bottom_sheet);
    BottomSheetBehavior behavior = BottomSheetBehavior.from(Objects.requireNonNull(bottomSheet));
    behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
  }

  private void initChildren(@NonNull View root)
  {
    initCloseBtn(root);
    initRecycler(root);
  }

  private void initCloseBtn(@NonNull View root)
  {
    View closeBtn = root.findViewById(R.id.сlose_btn);
    closeBtn.setOnClickListener(v -> dismiss());
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
                                                   new IsolinesItemClickListener(),
                                                   new GuidesItemClickListener()));
    recycler.setAdapter(mAdapter);
  }

  public static void show(@NonNull AppCompatActivity activity)
  {
    ToggleMapLayerDialog frag = new ToggleMapLayerDialog();
    String tag = frag.getClass().getCanonicalName();
    FragmentManager fm = activity.getSupportFragmentManager();

    Fragment oldInstance = fm.findFragmentByTag(tag);
    if (oldInstance != null)
      return;

    fm.beginTransaction().add(frag, tag).commitAllowingStateLoss();
    fm.executePendingTransactions();
  }

  @NonNull
  public View getRootView()
  {
    return mRoot;
  }

  @Nullable
  public static ToggleMapLayerDialog getInstance(@NonNull AppCompatActivity activity) {
    String tag = ToggleMapLayerDialog.class.getCanonicalName();
    return (ToggleMapLayerDialog) activity.getSupportFragmentManager().findFragmentByTag(tag);
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
      OnSubwayLayerToggleListener listener = (OnSubwayLayerToggleListener) requireActivity();
      listener.onSubwayLayerSelected();
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
    {
      OnTrafficLayerToggleListener listener = (OnTrafficLayerToggleListener) requireActivity();
      listener.onTrafficLayerSelected();
    }
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
      super.onItemClickInternal(v,item);
      OnIsolinesLayerToggleListener listener = (OnIsolinesLayerToggleListener) requireActivity();
      listener.onIsolinesLayerSelected();
    }
  }

  private class GuidesItemClickListener extends AbstractGuidesClickListener
  {
    private GuidesItemClickListener()
    {
      super(mAdapter, (NoConnectionListener) requireActivity());
    }

    @Override
    public void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      OnGuidesLayerToggleListener listener = (OnGuidesLayerToggleListener) requireActivity();
      listener.onGuidesLayerSelected();
    }
  }
}
