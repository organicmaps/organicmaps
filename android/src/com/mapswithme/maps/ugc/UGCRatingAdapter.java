package com.mapswithme.maps.ugc;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RatingBar;
import android.widget.TextView;

import com.mapswithme.maps.R;

import java.util.ArrayList;
import java.util.List;

class UGCRatingAdapter extends RecyclerView.Adapter<UGCRatingAdapter.ViewHolder>
{
  private static final int MAX_COUNT = 3;

  @NonNull
  private ArrayList<UGC.Rating> mItems = new ArrayList<>();

  @Override
  public UGCRatingAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new UGCRatingAdapter.ViewHolder(LayoutInflater.from(parent.getContext())
                                                         .inflate(R.layout.item_ugc_rating, parent, false));
  }

  @Override
  public void onBindViewHolder(UGCRatingAdapter.ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public int getItemCount()
  {
    return Math.min(mItems.size(), MAX_COUNT);
  }

  public void setItems(@NonNull List<UGC.Rating> items)
  {
    this.mItems.clear();
    this.mItems.addAll(items);
    notifyDataSetChanged();
  }

  static class ViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    final TextView mName;
    @NonNull
    final RatingBar mBar;

    public ViewHolder(View itemView)
    {
      super(itemView);
      mName = (TextView) itemView.findViewById(R.id.tv__name);
      mBar = (RatingBar) itemView.findViewById(R.id.rb__rate);
    }

    public void bind(UGC.Rating rating)
    {
      mName.setText(rating.getName());
      mBar.setRating(rating.getValue());
    }
  }
}
