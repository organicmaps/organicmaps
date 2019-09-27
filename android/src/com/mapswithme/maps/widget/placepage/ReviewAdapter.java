package com.mapswithme.maps.widget.placepage;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.review.Review;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

class ReviewAdapter extends RecyclerView.Adapter<ReviewAdapter.ViewHolder>
{
  private static final int MAX_COUNT = 3;

  @NonNull
  private ArrayList<Review> mItems = new ArrayList<>();

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return  new ViewHolder(LayoutInflater.from(parent.getContext())
                                         .inflate(R.layout.item_comment, parent, false));
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position), position > 0);
  }

  @Override
  public int getItemCount()
  {
    return Math.min(mItems.size(), MAX_COUNT);
  }

  public void setItems(@NonNull ArrayList<Review> items)
  {
    this.mItems = items;
    notifyDataSetChanged();
  }

  @NonNull
  public ArrayList<Review> getItems()
  {
    return mItems;
  }

  static class ViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    final View mDivider;
    @NonNull
    final TextView mUserName;
    @NonNull
    final TextView mCommentDate;
    @NonNull
    final TextView mRating;
    @NonNull
    final View mPositiveReview;
    @NonNull
    final TextView mTvPositiveReview;
    @NonNull
    final View mNegativeReview;
    @NonNull
    final TextView mTvNegativeReview;

    public ViewHolder(View view)
    {
      super(view);
      mDivider = view.findViewById(R.id.v__divider);
      mUserName = (TextView) view.findViewById(R.id.tv__user_name);
      mCommentDate = (TextView) view.findViewById(R.id.tv__comment_date);
      mRating = (TextView) view.findViewById(R.id.tv__user_rating);
      mPositiveReview = view.findViewById(R.id.ll__positive_review);
      mTvPositiveReview = (TextView) view.findViewById(R.id.tv__positive_review);
      mNegativeReview = view.findViewById(R.id.ll__negative_review);
      mTvNegativeReview = (TextView) view.findViewById(R.id.tv__negative_review);
    }

    public void bind(Review item, boolean isShowDivider)
    {
      UiUtils.showIf(isShowDivider, mDivider);
      mUserName.setText(item.getAuthor());
      Date date = new Date(item.getDate());
      mCommentDate.setText(DateFormat.getMediumDateFormat(mCommentDate.getContext()).format(date));
      mRating.setText(String.format(Locale.getDefault(), "%.1f", item.getRating()));
      if (TextUtils.isEmpty(item.getPros()))
      {
        UiUtils.hide(mPositiveReview);
      }
      else
      {
        UiUtils.show(mPositiveReview);
        mTvPositiveReview.setText(item.getPros());
      }
      if (TextUtils.isEmpty(item.getCons()))
      {
        UiUtils.hide(mNegativeReview);
      }
      else
      {
        UiUtils.show(mNegativeReview);
        mTvNegativeReview.setText(item.getCons());
      }
    }
  }
}
