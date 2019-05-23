package com.mapswithme.maps.discovery;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;

public class MoreAdapter extends RecyclerView.Adapter<MoreAdapter.MoreHolder>
{
  private static final int ITEMS_COUNT = 1;

  @NonNull
  private final View.OnClickListener mClickListener;

  public MoreAdapter(@NonNull OnItemClickListener<String> itemClickListener)
  {
    mClickListener = v -> itemClickListener.onItemClick(v, "");
  }

  @NonNull
  @Override
  public MoreHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View root = inflater.inflate(R.layout.item_search_more, parent, false);
    return new MoreHolder(root);
  }

  @Override
  public void onBindViewHolder(@NonNull MoreHolder holder, int position)
  {
    holder.itemView.setOnClickListener(mClickListener);
    holder.mTitle.setText(R.string.placepage_more_button);
  }

  @Override
  public int getItemCount()
  {
    return ITEMS_COUNT;
  }

  static class MoreHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mTitle;

    MoreHolder(@NonNull View itemView)
    {
      super(itemView);
      mTitle = itemView.findViewById(R.id.tv__title);
    }
  }
}
