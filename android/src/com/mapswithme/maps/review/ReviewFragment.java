package com.mapswithme.maps.review;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;

import java.util.ArrayList;

public class ReviewFragment extends BaseMwmFragment implements RecyclerClickListener
{
  @Nullable
  private ArrayList<Review> mItems;
  @Nullable
  private String mRating;
  private int mRatingBase;
  @Nullable
  private String mUrl;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_review, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    readArguments();

    if (mItems != null && mRating != null)
    {
      RecyclerView rvGallery = (RecyclerView) view.findViewById(R.id.rv__review);
      rvGallery.setLayoutManager(new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false));
      rvGallery.setAdapter(new ReviewAdapter(mItems, this, mRating, mRatingBase));
    }
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
  public void onItemClick(View v, int position)
  {
    if (mUrl == null)
      return;

    final Intent intent = new Intent(Intent.ACTION_VIEW);
    String url = mUrl;
    if (!url.startsWith("http://") && !url.startsWith("https://"))
      url = "http://" + url;
    intent.setData(Uri.parse(url));
    getContext().startActivity(intent);
  }
}
