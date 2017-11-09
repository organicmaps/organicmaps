package com.mapswithme.maps.ugc;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.Adapter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.RatingView;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

class UGCReviewAdapter extends Adapter<UGCReviewAdapter.ViewHolder>
{
  static final int MAX_COUNT = 3;
  static final DateFormat DATE_FORMATTER = new SimpleDateFormat("dd MMMM yyyy", Locale.getDefault());
  @NonNull
  private ArrayList<UGC.Review> mItems = new ArrayList<>();

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(parent.getContext())
                                        .inflate(R.layout.item_ugc_comment, parent, false));
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public int getItemCount()
  {
    return Math.min(mItems.size(), MAX_COUNT);
  }

  public void setItems(@NonNull List<UGC.Review> items)
  {
    this.mItems.clear();
    this.mItems.addAll(items);
    notifyDataSetChanged();
  }

  static class ViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    final TextView mAuthor;
    @NonNull
    final TextView mCommentDate;
    @NonNull
    final TextView mReview;
    @NonNull
    final RatingView mRating;

    public ViewHolder(View itemView)
    {
      super(itemView);
      mAuthor = (TextView) itemView.findViewById(R.id.name);
      mCommentDate = (TextView) itemView.findViewById(R.id.date);
      mReview = (TextView) itemView.findViewById(R.id.review);
      mRating = (RatingView) itemView.findViewById(R.id.rating);
      // TODO: remove "gone" visibility when review rating behaviour is fixed on the server.
      mRating.setVisibility(View.GONE);
    }

    public void bind(UGC.Review review)
    {
      mAuthor.setText(review.getAuthor());
      mCommentDate.setText(DATE_FORMATTER.format(new Date(review.getTime())));
      mReview.setText(review.getText());
      mRating.setRating(Impress.values()[review.getImpress()], String.valueOf(review.getRating()));
    }
  }
}
