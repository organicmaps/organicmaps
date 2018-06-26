package com.mapswithme.maps.subway;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.BottomSheetDialog;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.TextView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.SpanningLinearLayoutManager;
import com.mapswithme.maps.bookmarks.OnItemClickListener;

import java.util.Arrays;
import java.util.List;

public class SubwayTrafficToggleDialogFragment extends android.support.v4.app.DialogFragment implements OnItemClickListener<Mode>
{
  @NonNull
  @SuppressWarnings("NullableProblems")
  private View mRoot;

  @NonNull
  @SuppressWarnings("NullableProblems")
  private ModeAdapter mAdapter;

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    BottomSheetDialog dialog = new BottomSheetDialog(getActivity());
    LayoutInflater inflater = getActivity().getLayoutInflater();
    mRoot = inflater.inflate(R.layout.fragment_subway_traffic_toggle, null, false);
    dialog.setContentView(mRoot);
    initChildren();
    return dialog;
  }

  private void initChildren()
  {
    View closeBtn = mRoot.findViewById(R.id.сlose_btn);
    closeBtn.setOnClickListener(v -> dismiss());

    RecyclerView recycler = mRoot.findViewById(R.id.recycler);
    RecyclerView.LayoutManager layoutManager = new SpanningLinearLayoutManager(getContext(),
                                                                               LinearLayoutManager.HORIZONTAL,
                                                                               false);
    recycler.setLayoutManager(layoutManager);
    mAdapter = new ModeAdapter(Arrays.asList(Mode.values()), this);
    recycler.setAdapter(mAdapter);
  }

  public static void show(AppCompatActivity activity)
  {
    SubwayTrafficToggleDialogFragment frag = new SubwayTrafficToggleDialogFragment();
    String tag = frag.getClass().getCanonicalName();
    FragmentManager fm = activity.getSupportFragmentManager();

    Fragment oldInstance = fm.findFragmentByTag(tag);
    if (oldInstance != null)
      fm.beginTransaction().remove(oldInstance).commit();

    frag.show(fm, tag);
    fm.executePendingTransactions();
  }

  @Override
  public void onItemClick(@NonNull View v, @NonNull Mode item)
  {
    item.getItem().onSelected((MwmActivity)getActivity());
    mAdapter.notifyDataSetChanged();
  }

  private static class ModeAdapter extends RecyclerView.Adapter<ModeHolder>
  {
    @NonNull
    private final List<Mode> mModes;
    @NonNull
    private final OnItemClickListener<Mode> mListener;

    private ModeAdapter(@NonNull List<Mode> modes,
                        @NonNull OnItemClickListener<Mode> listener)
    {
      mModes = modes;
      mListener = listener;
    }

    @Override
    public ModeHolder onCreateViewHolder(ViewGroup parent, int viewType)
    {
      LayoutInflater inflater = LayoutInflater.from(parent.getContext());
      View root = inflater.inflate(R.layout.bootsheet_dialog_item, parent, false);
      return new ModeHolder(root, mListener);
    }

    @Override
    public void onBindViewHolder(ModeHolder holder, int position)
    {
      Context context = holder.itemView.getContext();
      Mode mode = mModes.get(position);
      holder.mItem = mode;
      holder.mButton.setSelected(mode.getItem().isSelected(context));
      holder.mButton.setImageResource(mode.getItem().getDrawableResId(context));
      holder.mTitle.setText(mode.getItem().getTitleResId());
    }

    @Override
    public int getItemCount()
    {
      return mModes.size();
    }
  }

  private static class ModeHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final ImageButton mButton;
    @NonNull
    private final OnItemClickListener<Mode> mListener;
    @NonNull
    private final TextView mTitle;
    @NonNull
    private Mode mItem;

    public ModeHolder(@NonNull View root, @NonNull OnItemClickListener<Mode> listener)
    {
      super(root);
      mButton = root.findViewById(R.id.item_btn);
      mTitle = root.findViewById(R.id.name);
      mListener = listener;
      mButton.setOnClickListener(this::onItemClicked);
    }

    private void onItemClicked(View v)
    {
      mListener.onItemClick(v, mItem);
    }
  }
}
