package com.mapswithme.maps.ugc;

import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RatingBar;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

import java.util.ArrayList;
import java.util.List;

class UGCRatingAdapter extends RecyclerView.Adapter<UGCRatingAdapter.ViewHolder>
{
  @NonNull
  private ArrayList<UGC.Rating> mItems = new ArrayList<>();

  @Override
  public UGCRatingAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    View itemView = LayoutInflater.from(parent.getContext())
                                  .inflate(R.layout.item_ugc_rating, parent, false);
    return new UGCRatingAdapter.ViewHolder(itemView);
  }

  @Override
  public void onBindViewHolder(UGCRatingAdapter.ViewHolder holder, int position)
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
    mItems.clear();
    mItems.addAll(items);
    notifyDataSetChanged();
  }

  @NonNull
  public List<UGC.Rating> getItems()
  {
    return mItems;
  }

  class ViewHolder extends RecyclerView.ViewHolder
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
      mBar.setOnRatingBarChangeListener(new RatingBar.OnRatingBarChangeListener()
      {
        @Override
        public void onRatingChanged(RatingBar ratingBar, float rating, boolean fromUser)
        {
          int position = getAdapterPosition();
          if (position >= mItems.size())
            throw new AssertionError("Adapter position must be in range [0; mItems.size() - 1]!");
          UGC.Rating item = mItems.get(position);
          item.setValue(rating);
        }
      });
    }

    public void bind(UGC.Rating rating)
    {
      @StringRes
      int nameId = Utils.getStringIdByKey(mName.getContext(), rating.getName());
      if (nameId != Utils.INVALID_ID)
        mName.setText(nameId);
      mBar.setRating(rating.getValue());
    }
  }
}
