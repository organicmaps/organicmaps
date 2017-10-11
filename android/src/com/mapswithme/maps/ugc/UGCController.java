package com.mapswithme.maps.ugc;

import android.app.Activity;
import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.RatingView;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.List;

public class UGCController implements View.OnClickListener, UGC.UGCListener
{
  @NonNull
  private final View mUgcRootView;
  @NonNull
  private final View mUgcAddRatingView;
  @NonNull
  private final View mUgcMoreReviews;
  @NonNull
  private final UGCReviewAdapter mUGCReviewAdapter = new UGCReviewAdapter();
  @NonNull
  private final UGCRatingRecordsAdapter mUGCRatingRecordsAdapter = new UGCRatingRecordsAdapter();
  @NonNull
  private final UGCRatingRecordsAdapter mUGCUserRatingRecordsAdapter = new UGCRatingRecordsAdapter();
  @NonNull
  private final TextView mReviewCount;
  @NonNull
  private final PlacePageView mPlacePage;
  @NonNull
  private final Button mLeaveReviewButton;
  @NonNull
  private final View mPreviewUgcInfoView;
  @NonNull
  private final View.OnClickListener mLeaveReviewClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      if (mMapObject == null)
        return;

      UGCEditorActivity.start((Activity) mPlacePage.getContext(), mMapObject.getTitle(),
                              mMapObject.getFeatureId(),
                              UGC.getUserRatings(), UGC.RATING_NONE, mMapObject.canBeReviewed());
    }
  };

  @Nullable
  private MapObject mMapObject;
  @Nullable
  private UGC mUgc;

  public UGCController(@NonNull final PlacePageView placePage)
  {
    mPlacePage = placePage;
    final Context context = mPlacePage.getContext();
    mUgcRootView = mPlacePage.findViewById(R.id.ll__pp_ugc);
    mPreviewUgcInfoView = mPlacePage.findViewById(R.id.preview_rating_info);
    mUgcMoreReviews = mPlacePage.findViewById(R.id.tv__pp_ugc_reviews_more);
    mUgcAddRatingView = mPlacePage.findViewById(R.id.ll__pp_ugc_add_rating);
    mUgcAddRatingView.findViewById(R.id.ll__horrible).setOnClickListener(this);
    mUgcAddRatingView.findViewById(R.id.ll__bad).setOnClickListener(this);
    mUgcAddRatingView.findViewById(R.id.ll__normal).setOnClickListener(this);
    mUgcAddRatingView.findViewById(R.id.ll__good).setOnClickListener(this);
    mUgcAddRatingView.findViewById(R.id.ll__excellent).setOnClickListener(this);
    mReviewCount = (TextView) mPlacePage.findViewById(R.id.tv__review_count);
    mLeaveReviewButton = (Button) mPlacePage.findViewById(R.id.leaveReview);
    mLeaveReviewButton.setOnClickListener(mLeaveReviewClickListener);

    RecyclerView rvUGCReviews = (RecyclerView) mPlacePage.findViewById(R.id.rv__pp_ugc_reviews);
    rvUGCReviews.setLayoutManager(new LinearLayoutManager(context));
    rvUGCReviews.getLayoutManager().setAutoMeasureEnabled(true);
    rvUGCReviews.addItemDecoration(
        ItemDecoratorFactory.createDefaultDecorator(context, LinearLayoutManager.VERTICAL));
    rvUGCReviews.setNestedScrollingEnabled(false);
    rvUGCReviews.setHasFixedSize(false);
    rvUGCReviews.setAdapter(mUGCReviewAdapter);

    View summaryRatingContainer = mPlacePage.findViewById(R.id.summary_rating_records);
    RecyclerView rvRatingRecords = (RecyclerView) summaryRatingContainer.findViewById(R.id.rv__summary_rating_records);
    rvRatingRecords.setLayoutManager(new LinearLayoutManager(context, LinearLayoutManager.HORIZONTAL, false));
    rvRatingRecords.getLayoutManager().setAutoMeasureEnabled(true);
    rvRatingRecords.setNestedScrollingEnabled(false);
    rvRatingRecords.setHasFixedSize(false);
    rvRatingRecords.addItemDecoration(
        ItemDecoratorFactory.createRatingRecordDecorator(context, LinearLayoutManager.HORIZONTAL));
    rvRatingRecords.setAdapter(mUGCRatingRecordsAdapter);

    View userReviewContainer = mPlacePage.findViewById(R.id.user_rating_records);
    RecyclerView rvUserRatingRecords = (RecyclerView) userReviewContainer.findViewById(R.id.rv__summary_rating_records);
    rvUserRatingRecords.setLayoutManager(new LinearLayoutManager(context, LinearLayoutManager.HORIZONTAL, false));
    rvUserRatingRecords.getLayoutManager().setAutoMeasureEnabled(true);
    rvUserRatingRecords.setNestedScrollingEnabled(false);
    rvUserRatingRecords.setHasFixedSize(false);
    rvUserRatingRecords.addItemDecoration(
        ItemDecoratorFactory.createRatingRecordDecorator(context, LinearLayoutManager.HORIZONTAL));
    rvUserRatingRecords.setAdapter(mUGCUserRatingRecordsAdapter);

    View userComment = mPlacePage.findViewById(R.id.rl_user_review);
    TextView name = (TextView) userComment.findViewById(R.id.name);
    TextView date = (TextView) userComment.findViewById(R.id.date);
    TextView review = (TextView) userComment.findViewById(R.id.review);
    //TODO: remove it after core is ready.
    name.setText("Your review");
    date.setText("10 May 2017");
    review.setText("Go first thing in the morning when they open...You will get in right" +
                   "away if you do..." +
                   "" +
                   "Amazing food...");
    userComment.findViewById(R.id.rating).setVisibility(View.GONE);

    UGC.setListener(this);
  }

  public void clear()
  {
    mUGCReviewAdapter.setItems(new ArrayList<UGC.Review>());
    mUGCRatingRecordsAdapter.setItems(new ArrayList<UGC.Rating>());
    mUGCUserRatingRecordsAdapter.setItems(new ArrayList<UGC.Rating>());
    mReviewCount.setText("");
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId()){
      case R.id.ll__horrible:
        onAggRatingTapped(UGC.RATING_HORRIBLE);
        break;
      case R.id.ll__bad:
        onAggRatingTapped(UGC.RATING_BAD);
        break;
      case R.id.ll__normal:
        onAggRatingTapped(UGC.RATING_NORMAL);
        break;
      case R.id.ll__good:
        onAggRatingTapped(UGC.RATING_GOOD);
        break;
      case R.id.ll__excellent:
        onAggRatingTapped(UGC.RATING_EXCELLENT);
        break;
      default:
        throw new AssertionError("Unknown rating view:");
    }
  }

  public void getUGC(@NonNull MapObject mapObject)
  {
    mMapObject = mapObject;
    if (mapObject.shouldShowUGC())
      UGC.requestUGC(mMapObject.getFeatureId());
    else
      UiUtils.hide(mUgcRootView, mLeaveReviewButton);
  }

  public boolean isLeaveReviewButtonTouched(@NonNull MotionEvent event)
  {
    return UiUtils.isViewTouched(event, mLeaveReviewButton);
  }

  @Override
  public void onUGCReceived(@Nullable UGC ugc, @Nullable UGCUpdate ugcUpdate, @UGC.Impress int impress,
                            @NonNull String rating)
  {
    UiUtils.show(mPreviewUgcInfoView);
    RatingView ratingView = (RatingView) mPreviewUgcInfoView.findViewById(R.id.rating_view);
    ratingView.setRating(Impress.values()[impress], rating);
    UiUtils.showIf(ugcUpdate == null, mLeaveReviewButton);
    mUgc = ugc;
    if (mUgc == null)
    {
      mReviewCount.setText(ugcUpdate != null ? R.string.placepage_reviewed : R.string.placepage_no_reviews);
      return;
    }

    List<UGC.Review> reviews = ugc.getReviews();
    if (reviews != null)
      mUGCReviewAdapter.setItems(ugc.getReviews());
    mUGCRatingRecordsAdapter.setItems(ugc.getRatings());
    mUGCUserRatingRecordsAdapter.setItems(ugc.getUserRatings());
    Context context = mPlacePage.getContext();
    mReviewCount.setText(context.getString(R.string.placepage_summary_rating_description,
                                           String.valueOf(mUgc.getBasedOnCount())));
    UiUtils.showIf(reviews != null && reviews.size() > UGCReviewAdapter.MAX_COUNT, mUgcMoreReviews);
    UiUtils.showIf(mMapObject != null && mMapObject.canBeRated(), mUgcAddRatingView);
    UiUtils.show(mUgcRootView);
  }

  private void onAggRatingTapped(@UGC.Impress int rating)
  {
    if (mMapObject == null || mUgc == null)
      return;

    UGCEditorActivity.start((Activity) mPlacePage.getContext(), mMapObject.getTitle(),
                            mMapObject.getFeatureId(),
                            mUgc.getUserRatings(), rating, mMapObject.canBeReviewed());
  }
}
