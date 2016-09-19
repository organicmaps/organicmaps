package com.mapswithme.maps.review;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.widget.placepage.SponsoredHotel;
import com.mapswithme.util.UiUtils;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.widget.Toolbar;

import java.util.ArrayList;

public class ReviewActivity extends BaseMwmFragmentActivity {
  static final String EXTRA_REVIEWS = "review_items";
  static final String EXTRA_TITLE = "review_title";
  static final String EXTRA_RATING = "review_rating";
  static final String EXTRA_RATING_BASE = "review_rating_base";
  static final String EXTRA_RATING_URL = "review_rating_url";

  public static void start(Context context, ArrayList<SponsoredHotel.Review> items, String title,
          String rating, int ratingBase, String url)
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
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    String title = "";
    Bundle bundle = getIntent().getExtras();
    if (bundle != null) {
      title = bundle.getString(EXTRA_TITLE);
    }
    Toolbar toolbar = getToolbar();
    toolbar.setTitle(title);
    UiUtils.showHomeUpButton(toolbar);
    displayToolbarAsActionBar();
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass() {
    return ReviewFragment.class;
  }

  @Override
  protected int getContentLayoutResId() {
    return R.layout.activity_fragment_and_toolbar;
  }

  @Override
  protected int getFragmentContentResId() {
    return R.id.fragment_container;
  }
}
