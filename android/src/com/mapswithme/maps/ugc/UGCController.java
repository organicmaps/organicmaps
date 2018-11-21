package com.mapswithme.maps.ugc;

import android.app.Activity;
import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.RatingView;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.DateUtils;
import com.mapswithme.util.UiUtils;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Date;
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
  private final View mUserRatingRecordsContainer;
  @NonNull
  private final View mUserReviewView;
  @NonNull
  private final View mUserReviewDivider;
  @NonNull
  private final View mReviewListDivider;
  @NonNull
  private final View mSummaryRootView;
  @NonNull
  private final TextView mSummaryReviewCount;
  @NonNull
  private final View.OnClickListener mLeaveReviewClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      if (mMapObject == null)
        return;

      EditParams.Builder builder = EditParams.Builder.fromMapObject(mMapObject)
                                                     .setDefaultRating(UGC.RATING_NONE)
                                                     .setFromPP(true);

      UGCEditorActivity.start((Activity) mPlacePage.getContext(), builder.build());
    }
  };
  @NonNull
  private final View.OnClickListener mMoreReviewsClickListener = new View.OnClickListener()
  {

    @Override
    public void onClick(View v)
    {
      // TODO: coming soon.
    }
  };

  @Nullable
  private MapObject mMapObject;

  public UGCController(@NonNull final PlacePageView placePage)
  {
    mPlacePage = placePage;
    final Context context = mPlacePage.getContext();
    mUgcRootView = mPlacePage.findViewById(R.id.ll__pp_ugc);
    mPreviewUgcInfoView = mPlacePage.findViewById(R.id.preview_rating_info);
    mUgcMoreReviews = mPlacePage.findViewById(R.id.tv__pp_ugc_reviews_more);
    mUgcMoreReviews.setOnClickListener(mMoreReviewsClickListener);
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

    mSummaryRootView = mPlacePage.findViewById(R.id.ll__summary_container);
    View summaryRatingContainer = mSummaryRootView.findViewById(R.id.summary_rating_records);
    RecyclerView rvRatingRecords = (RecyclerView) summaryRatingContainer.findViewById(R.id.rv__summary_rating_records);
    rvRatingRecords.setLayoutManager(new LinearLayoutManager(context, LinearLayoutManager.HORIZONTAL, false));
    rvRatingRecords.getLayoutManager().setAutoMeasureEnabled(true);
    rvRatingRecords.setNestedScrollingEnabled(false);
    rvRatingRecords.setHasFixedSize(false);
    rvRatingRecords.addItemDecoration(
        ItemDecoratorFactory.createRatingRecordDecorator(context, LinearLayoutManager.HORIZONTAL));
    rvRatingRecords.setAdapter(mUGCRatingRecordsAdapter);
    mSummaryReviewCount = (TextView) mSummaryRootView.findViewById(R.id.tv__review_count);

    mUserRatingRecordsContainer = mPlacePage.findViewById(R.id.user_rating_records);
    RecyclerView userRatingRecords = (RecyclerView) mUserRatingRecordsContainer.findViewById(R.id.rv__summary_rating_records);
    userRatingRecords.setLayoutManager(new LinearLayoutManager(context, LinearLayoutManager.HORIZONTAL, false));
    userRatingRecords.getLayoutManager().setAutoMeasureEnabled(true);
    userRatingRecords.setNestedScrollingEnabled(false);
    userRatingRecords.setHasFixedSize(false);
    userRatingRecords.addItemDecoration(
        ItemDecoratorFactory.createRatingRecordDecorator(context, LinearLayoutManager.HORIZONTAL));
    userRatingRecords.setAdapter(mUGCUserRatingRecordsAdapter);

    mUserReviewView = mPlacePage.findViewById(R.id.rl_user_review);
    mUserReviewView.findViewById(R.id.rating).setVisibility(View.GONE);
    mReviewListDivider = mPlacePage.findViewById(R.id.ugc_review_list_divider);
    mUserReviewDivider = mPlacePage.findViewById(R.id.user_review_divider);

    UGC.setListener(this);
  }

  public void clear()
  {
    UiUtils.hide(mUgcRootView, mLeaveReviewButton, mPreviewUgcInfoView);
    mUGCReviewAdapter.setItems(new ArrayList<UGC.Review>());
    mUGCRatingRecordsAdapter.setItems(new ArrayList<UGC.Rating>());
    mUGCUserRatingRecordsAdapter.setItems(new ArrayList<UGC.Rating>());
    mReviewCount.setText("");
    mSummaryReviewCount.setText("");
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
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
    if (!mapObject.shouldShowUGC())
      return;

    mMapObject = mapObject;
    UGC.requestUGC(mMapObject.getFeatureId());
  }

  public boolean isLeaveReviewButtonTouched(@NonNull MotionEvent event)
  {
    return UiUtils.isViewTouched(event, mLeaveReviewButton);
  }

  @Override
  public void onUGCReceived(@Nullable UGC ugc, @Nullable UGCUpdate ugcUpdate, @UGC.Impress int impress,
                            @NonNull String rating)
  {
    UiUtils.show(mUgcRootView);
    UiUtils.showIf(ugc != null || canUserRate(ugcUpdate) || ugcUpdate != null, mPreviewUgcInfoView);
    UiUtils.showIf(canUserRate(ugcUpdate), mLeaveReviewButton, mUgcAddRatingView);
    UiUtils.showIf(ugc != null, mUgcMoreReviews);
    UiUtils.showIf(ugc != null && impress != UGC.RATING_NONE, mSummaryRootView);
    RatingView ratingView = (RatingView) mPreviewUgcInfoView.findViewById(R.id.rating_view);
    if (ugc == null)
    {
      mReviewCount.setText(ugcUpdate != null ? R.string.placepage_reviewed : R.string.placepage_no_reviews);
      ratingView.setRating(ugcUpdate == null ? Impress.NONE : Impress.COMING_SOON, rating);
      setUserReviewAndRatingsView(ugcUpdate);
      return;
    }

    Context context = mPlacePage.getContext();
    int reviewsCount = ugc.getBasedOnCount();
    if (impress != UGC.RATING_NONE)
    {
      mReviewCount.setText(context.getResources().getQuantityString(
          R.plurals.placepage_summary_rating_description, reviewsCount, reviewsCount));
      setSummaryViews(ugc, impress, rating);
    }
    ratingView.setRating(Impress.values()[impress], rating);
    setUserReviewAndRatingsView(ugcUpdate);
    List<UGC.Review> reviews = ugc.getReviews();
    if (reviews != null)
      mUGCReviewAdapter.setItems(ugc.getReviews());
    UiUtils.showIf(reviews != null, mReviewListDivider);
    // TODO: don't show "more reviews" button while reviews screen activity is not ready.
    UiUtils.showIf(false /* reviews != null && reviews.size() > UGCReviewAdapter.MAX_COUNT */,
                   mUgcMoreReviews);
  }

  private void setSummaryViews(@NonNull UGC ugc, @UGC.Impress int impress, @NonNull String rating)
  {
    RatingView ratingView = (RatingView) mSummaryRootView.findViewById(R.id.rv__summary_rating);
    ratingView.setRating(Impress.values()[impress], rating);
    Context context = mPlacePage.getContext();
    int reviewsCount = ugc.getBasedOnCount();
    mSummaryReviewCount.setText(context.getResources().getQuantityString(
        R.plurals.placepage_summary_rating_description, reviewsCount, reviewsCount));
    mUGCRatingRecordsAdapter.setItems(ugc.getRatings());
  }

  private boolean canUserRate(@Nullable UGCUpdate ugcUpdate)
  {
    return mMapObject != null && mMapObject.canBeRated() && ugcUpdate == null;
  }

  private void onAggRatingTapped(@UGC.Impress int rating)
  {
    if (mMapObject == null)
      return;

    EditParams.Builder builder = EditParams.Builder.fromMapObject(mMapObject)
                                                   .setDefaultRating(rating);
    UGCEditorActivity.start((Activity) mPlacePage.getContext(), builder.build());
  }

  private void setUserReviewAndRatingsView(@Nullable UGCUpdate update)
  {
    UiUtils.showIf(update != null, mUserReviewView, mUserReviewDivider,
                   mUserRatingRecordsContainer);
    if (update == null)
      return;
    TextView name = mUserReviewView.findViewById(R.id.name);
    TextView date = mUserReviewView.findViewById(R.id.date);
    name.setText(R.string.placepage_reviews_your_comment);
    DateFormat formatter = DateUtils.getMediumDateFormatter();
    date.setText(formatter.format(new Date(update.getTimeMillis())));
    TextView review = mUserReviewView.findViewById(R.id.review);
    UiUtils.showIf(!TextUtils.isEmpty(update.getText()), review);
    review.setText(update.getText());
    mUGCUserRatingRecordsAdapter.setItems(update.getRatings());
  }
}
