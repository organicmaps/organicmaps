package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.review.Review;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

class ReviewAdapter extends BaseAdapter
{
  private static final int MAX_COUNT = 3;

  @NonNull
  private ArrayList<Review> mItems = new ArrayList<>();

  @Override
  public int getCount()
  {
    return Math.min(mItems.size(), MAX_COUNT);
  }

  @Override
  public Object getItem(int position)
  {
    return mItems.get(position);
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    ViewHolder holder;
    if (convertView == null)
    {
      convertView = LayoutInflater.from(parent.getContext())
                                  .inflate(R.layout.item_comment, parent, false);
      holder = new ViewHolder(convertView);
      convertView.setTag(holder);
    }
    else
    {
      holder = (ViewHolder) convertView.getTag();
    }

    holder.bind(mItems.get(position), position > 0);

    return convertView;
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

  private static class ViewHolder
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
      if (TextUtils.isEmpty(item.getReviewPositive()))
      {
        UiUtils.hide(mPositiveReview);
      }
      else
      {
        UiUtils.show(mPositiveReview);
        mTvPositiveReview.setText(item.getReviewPositive());
      }
      if (TextUtils.isEmpty(item.getReviewNegative()))
      {
        UiUtils.hide(mNegativeReview);
      }
      else
      {
        UiUtils.show(mNegativeReview);
        mTvNegativeReview.setText(item.getReviewNegative());
      }
    }
  }
}
