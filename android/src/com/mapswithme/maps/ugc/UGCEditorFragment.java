package com.mapswithme.maps.ugc;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.BaseMwmAuthorizationFragment;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.List;

public class UGCEditorFragment extends BaseMwmAuthorizationFragment
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = UGCEditorFragment.class.getSimpleName();
  static final String ARG_FEATURE_ID = "arg_feature_id";
  static final String ARG_TITLE = "arg_title";
  static final String ARG_DEFAULT_RATING = "arg_default_rating";
  static final String ARG_RATING_LIST = "arg_rating_list";
  @NonNull
  private final UGCRatingAdapter mUGCRatingAdapter = new UGCRatingAdapter();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private EditText mReviewEditText;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_editor, container, false);
    mReviewEditText = (EditText) root.findViewById(R.id.review);

    RecyclerView rvRatingView = (RecyclerView) root.findViewById(R.id.ratings);
    rvRatingView.setLayoutManager(new LinearLayoutManager(getContext()));
    rvRatingView.getLayoutManager().setAutoMeasureEnabled(true);
    rvRatingView.setNestedScrollingEnabled(false);
    rvRatingView.setHasFixedSize(false);
    rvRatingView.setAdapter(mUGCRatingAdapter);
    Bundle args = getArguments();
    if (args == null)
      throw new AssertionError("Args must be passed to this fragment!");
    List<UGC.Rating> ratings = args.getParcelableArrayList(ARG_RATING_LIST);
    if (ratings != null)
      setDefaultRatingValue(args, ratings);
    return root;
  }

  private void setDefaultRatingValue(@NonNull Bundle args, @NonNull List<UGC.Rating> ratings)
  {
    for (UGC.Rating rating : ratings)
      rating.setValue(args.getInt(ARG_DEFAULT_RATING, 0));
    mUGCRatingAdapter.setItems(ratings);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController.setTitle(getArguments().getString(ARG_TITLE));
  }

  @Override
  protected void onSubmitButtonClick()
  {
    super.onSubmitButtonClick();
    List<UGC.Rating> modifiedRatings = mUGCRatingAdapter.getItems();
    UGC.Rating[] ratings = new UGC.Rating[modifiedRatings.size()];
    modifiedRatings.toArray(ratings);
    UGCUpdate update = new UGCUpdate(ratings, mReviewEditText.getText().toString());
    FeatureId featureId = getArguments().getParcelable(ARG_FEATURE_ID);
    if (featureId == null)
      throw new AssertionError("Feature ID must be passed to this fragment!");
    UGC.setUGCUpdate(featureId, update);
  }

  @Override
  protected void onPreSocialAuthentication()
  {
    LOGGER.i(TAG, "onPreSocialAuthentication()");
  }

  @Override
  protected void onAuthorized()
  {
    LOGGER.i(TAG, "onAuthorized()");
  }

  @Override
  protected void onStartAuthorization()
  {
    LOGGER.i(TAG, "onStartAuthorization()");
  }
}
