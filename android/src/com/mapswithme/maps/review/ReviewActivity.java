package com.mapswithme.maps.review;

import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import com.mapswithme.maps.base.BaseMwmExtraTitleActivity;

import java.util.ArrayList;

public class ReviewActivity extends BaseMwmExtraTitleActivity
{
  static final String EXTRA_REVIEWS = "review_items";
  static final String EXTRA_RATING = "review_rating";
  static final String EXTRA_RATING_BASE = "review_rating_base";
  static final String EXTRA_RATING_URL = "review_rating_url";

  public static void start(Context context, @NonNull ArrayList<Review> items,
                           @NonNull String title, @NonNull String rating, int ratingBase,
                           @NonNull String url)
  {
    final Intent i = new Intent(context, ReviewActivity.class);
    i.putParcelableArrayListExtra(EXTRA_REVIEWS, items);
    i.putExtra(EXTRA_TITLE, title);
    i.putExtra(EXTRA_RATING, rating);
    i.putExtra(EXTRA_RATING_BASE, ratingBase);
    i.putExtra(EXTRA_RATING_URL, url);
    context.startActivity(i);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return ReviewFragment.class;
  }
}
