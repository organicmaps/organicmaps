package com.mapswithme.maps.review;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.util.UiUtils;

import android.support.annotation.CallSuper;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

class ReviewAdapter extends RecyclerView.Adapter<ReviewAdapter.BaseViewHolder> {
  private static final int MAX_COUNT = 15;
  private static final int VIEW_TYPE_REVIEW = 0;
  private static final int VIEW_TYPE_MORE = 1;

  private final ArrayList<Review> mItems;
  private final RecyclerClickListener mListener;

  ReviewAdapter(ArrayList<Review> images, RecyclerClickListener listener) {
    mItems = images;
    mListener = listener;
  }

  @Override
  public BaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
    if (viewType == VIEW_TYPE_REVIEW) {
      return new ReviewHolder(LayoutInflater.from(parent.getContext())
              .inflate(R.layout.item_comment, parent, false), mListener);
    }

    return new MoreHolder(LayoutInflater.from(parent.getContext())
            .inflate(R.layout.item_more_button, parent, false), mListener);
  }

  @Override
  public void onBindViewHolder(BaseViewHolder holder, int position) {
    if (position < mItems.size()) {
      holder.bind(mItems.get(position), position);
    } else {
      holder.bind(null, position);
    }
  }

  @Override
  public int getItemCount() {
    if (mItems == null) {
      return 0;
    }
    if (mItems.size() > MAX_COUNT) {
      return MAX_COUNT + 1;
    }
    return mItems.size() + 1;
  }

  @Override
  public int getItemViewType(int position) {
    if (position == mItems.size()) {
      return VIEW_TYPE_MORE;
    }

    return VIEW_TYPE_REVIEW;
  }

  static abstract class BaseViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
    private final RecyclerClickListener mListener;
    private int mPosition;

    BaseViewHolder(View itemView, RecyclerClickListener listener) {
      super(itemView);
      mListener = listener;
    }

    @Override
    public void onClick(View v) {
      if (mListener != null) {
        mListener.onItemClick(v, mPosition);
      }
    }

    @CallSuper
    public void bind(Review item, int position) {
      mPosition = position;
    }
  }

  private static class ReviewHolder extends BaseViewHolder {
    final View mDivider;
    final TextView mUserName;
    final TextView mCommentDate;
    final TextView mRating;
    final View mPositiveReview;
    final TextView mTvPositiveReview;
    final View mNegativeReview;
    final TextView mTvNegativeReview;

    ReviewHolder(View itemView, RecyclerClickListener listener) {
      super(itemView, listener);
      mDivider = itemView.findViewById(R.id.v__divider);
      mUserName = (TextView) itemView.findViewById(R.id.tv__user_name);
      mCommentDate = (TextView) itemView.findViewById(R.id.tv__comment_date);
      mRating = (TextView) itemView.findViewById(R.id.tv__user_rating);
      mPositiveReview = itemView.findViewById(R.id.ll__positive_review);
      mTvPositiveReview = (TextView) itemView.findViewById(R.id.tv__positive_review);
      mNegativeReview = itemView.findViewById(R.id.ll__negative_review);
      mTvNegativeReview = (TextView) itemView.findViewById(R.id.tv__negative_review);
    }

    @Override
    public void bind(Review item, int position) {
      super.bind(item, position);
      UiUtils.showIf(position > 0, mDivider);
      mUserName.setText(item.getAuthor());
      Date date = new Date(item.getDate());
      mCommentDate.setText(DateFormat.getMediumDateFormat(mCommentDate.getContext()).format(date));
      mRating.setText(String.format(Locale.getDefault(), "%.1f", item.getRating()));
      if (TextUtils.isEmpty(item.getReviewPositive())) {
        UiUtils.hide(mPositiveReview);
      } else {
        UiUtils.show(mPositiveReview);
        mTvPositiveReview.setText(item.getReviewPositive());
      }
      if (TextUtils.isEmpty(item.getReviewNegative())) {
        UiUtils.hide(mNegativeReview);
      } else {
        UiUtils.show(mNegativeReview);
        mTvNegativeReview.setText(item.getReviewNegative());
      }
    }
  }

  private static class MoreHolder extends BaseViewHolder {

    MoreHolder(View itemView, RecyclerClickListener listener) {
      super(itemView, listener);
      itemView.setOnClickListener(this);
    }
  }
}
