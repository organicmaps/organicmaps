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

class UGCRatingRecordsAdapter extends RecyclerView.Adapter<UGCRatingRecordsAdapter.ViewHolder>
{
  @NonNull
  private ArrayList<UGC.Rating> mItems = new ArrayList<>();

  @Override
  public UGCRatingRecordsAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new UGCRatingRecordsAdapter.ViewHolder(LayoutInflater.from(parent.getContext())
                                                                .inflate(R.layout.item_ugc_rating_record, parent, false));
  }

  @Override
  public void onBindViewHolder(UGCRatingRecordsAdapter.ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
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
      mName = (TextView) itemView.findViewById(R.id.name);
      mBar = (RatingBar) itemView.findViewById(R.id.rating);
    }

    public void bind(UGC.Rating rating)
    {
      mName.setText(rating.getName());
      mBar.setRating(rating.getValue());
    }
  }
}
