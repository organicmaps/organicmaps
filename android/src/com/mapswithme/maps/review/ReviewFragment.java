package com.mapswithme.maps.review;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.widget.placepage.SponsoredHotel;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.ArrayList;

public class ReviewFragment extends BaseMwmFragment implements RecyclerClickListener {
  private ArrayList<SponsoredHotel.Review> mItems;
  private String mRating;
  private int mRatingBase;
  private String mUrl;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
          @Nullable Bundle savedInstanceState) {
    return inflater.inflate(R.layout.fragment_review, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    readArguments();

    RecyclerView rvGallery = (RecyclerView) view.findViewById(R.id.rv__review);
    rvGallery.setLayoutManager(new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false));
    rvGallery.setAdapter(new ReviewAdapter(mItems, this));
    TextView hotelRating = (TextView) view.findViewById(R.id.tv__place_hotel_rating);
    TextView hotelRatingBase = (TextView) view.findViewById(R.id.tv__place_hotel_rating_base);
    hotelRating.setText(mRating);
    hotelRatingBase.setText(getResources().getQuantityString(R.plurals.place_page_booking_rating_base,
            mRatingBase, mRatingBase));
  }

  private void readArguments()
  {
    final Bundle arguments = getArguments();
    if (arguments == null)
      return;

    mItems = arguments.getParcelableArrayList(ReviewActivity.EXTRA_REVIEWS);
    mRating = arguments.getString(ReviewActivity.EXTRA_RATING);
    mRatingBase = arguments.getInt(ReviewActivity.EXTRA_RATING_BASE);
    mUrl = arguments.getString(ReviewActivity.EXTRA_RATING_URL);
  }

  @Override
  public void onItemClick(View v, int position) {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    String url = mUrl;
    if (!url.startsWith("http://") && !url.startsWith("https://"))
      url = "http://" + url;
    intent.setData(Uri.parse(url));
    getContext().startActivity(intent);
  }
}
