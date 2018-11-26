package com.mapswithme.maps.maplayer;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.BottomSheetBehavior;
import android.support.design.widget.BottomSheetDialog;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.maps.maplayer.subway.OnSubwayLayerToggleListener;
import com.mapswithme.maps.maplayer.traffic.OnTrafficLayerToggleListener;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.maps.widget.recycler.SpanningLinearLayoutManager;

import java.util.Arrays;
import java.util.List;
import java.util.Objects;

public class ToggleMapLayerDialog extends DialogFragment
{
  @NonNull
  @SuppressWarnings("NullableProblems")
  private ModeAdapter mAdapter;

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    BottomSheetDialog dialog = new BottomSheetDialog(getActivity());
    LayoutInflater inflater = getActivity().getLayoutInflater();
    View root = inflater.inflate(R.layout.fragment_toggle_map_layer, null, false);
    dialog.setOnShowListener(this::onShow);
    dialog.setContentView(root);
    initChildren(root);
    return dialog;
  }

  private void onShow(@NonNull DialogInterface dialogInterface)
  {
    BottomSheetDialog dialog = (BottomSheetDialog) dialogInterface;
    View bottomSheet = dialog.findViewById(android.support.design.R.id.design_bottom_sheet);
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
    RecyclerView.LayoutManager layoutManager = new SpanningLinearLayoutManager(getContext(),
                                                                               LinearLayoutManager.HORIZONTAL,
                                                                               false);
    recycler.setLayoutManager(layoutManager);
    mAdapter = new ModeAdapter(createItems());
    recycler.setAdapter(mAdapter);
  }

  @NonNull
  private List<Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>>> createItems()
  {
    SubwayItemClickListener subwayListener = new SubwayItemClickListener();
    Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>> subway
        = new Pair<>(BottomSheetItem.Subway.makeInstance(getContext()), subwayListener);

    TrafficItemClickListener trafficListener = new TrafficItemClickListener();
    Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>> traffic
        = new Pair<>(BottomSheetItem.Traffic.makeInstance(getContext()), trafficListener);

    return Arrays.asList(traffic, subway);
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

  private static class ModeAdapter extends RecyclerView.Adapter<ModeHolder>
  {
    @NonNull
    private final List<Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>>> mItems;

    private ModeAdapter(@NonNull List<Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>>> modes)
    {
      mItems = modes;
    }

    @Override
    public ModeHolder onCreateViewHolder(ViewGroup parent, int viewType)
    {
      LayoutInflater inflater = LayoutInflater.from(parent.getContext());
      View root = inflater.inflate(R.layout.item_bottomsheet_dialog, parent, false);
      return new ModeHolder(root);
    }

    @Override
    public void onBindViewHolder(ModeHolder holder, int position)
    {
      Context context = holder.itemView.getContext();
      Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>> pair = mItems.get(position);
      BottomSheetItem item = pair.first;
      holder.mItem = item;

      boolean isEnabled = item.getMode().isEnabled(context);

      holder.mButton.setSelected(isEnabled);
      holder.mTitle.setSelected(isEnabled);
      holder.mTitle.setText(item.getTitle());
      holder.mButton.setImageResource(isEnabled ? item.getEnabledStateDrawable()
                                                : item.getDisabledStateDrawable());
      holder.mListener = pair.second;
    }

    @Override
    public int getItemCount()
    {
      return mItems.size();
    }
  }
  private static class ModeHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final ImageView mButton;
    @NonNull
    private final TextView mTitle;
    @Nullable
    private BottomSheetItem mItem;
    @Nullable
    private OnItemClickListener<BottomSheetItem> mListener;

    ModeHolder(@NonNull View root)
    {
      super(root);
      mButton = root.findViewById(R.id.btn);
      mTitle = root.findViewById(R.id.name);
      mButton.setOnClickListener(this::onItemClicked);
    }

    @NonNull
    public BottomSheetItem getItem()
    {
      return Objects.requireNonNull(mItem);
    }

    @NonNull
    public OnItemClickListener<BottomSheetItem> getListener()
    {
      return Objects.requireNonNull(mListener);
    }

    private void onItemClicked(@NonNull View v)
    {
      getListener().onItemClick(v, getItem());
    }
  }

  private abstract class DefaultClickListener implements OnItemClickListener<BottomSheetItem>
  {
    @Override
    public final void onItemClick(@NonNull View v, @NonNull BottomSheetItem item)
    {
      item.getMode().toggle(getContext());
      onItemClickInternal(v, item);
      mAdapter.notifyDataSetChanged();
    }

    abstract void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item);
  }

  private class SubwayItemClickListener extends DefaultClickListener
  {
    @Override
    void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      OnSubwayLayerToggleListener listener = (OnSubwayLayerToggleListener) getActivity();
      listener.onSubwayLayerSelected();
    }
  }

  private class TrafficItemClickListener extends DefaultClickListener
  {
    @Override
    void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
    {
      OnTrafficLayerToggleListener listener = (OnTrafficLayerToggleListener) getActivity();
      listener.onTrafficLayerSelected();
    }
  }
}
