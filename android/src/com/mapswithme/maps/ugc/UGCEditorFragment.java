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

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.BaseMwmAuthorizationFragment;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Language;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;

public class UGCEditorFragment extends BaseMwmAuthorizationFragment
{
  static final String ARG_FEATURE_ID = "arg_feature_id";
  static final String ARG_TITLE = "arg_title";
  static final String ARG_DEFAULT_RATING = "arg_default_rating";
  static final String ARG_RATING_LIST = "arg_rating_list";
  static final String ARG_CAN_BE_REVIEWED = "arg_can_be_reviewed";
  static final String ARG_LAT = "arg_lat";
  static final String ARG_LON = "arg_lon";
  static final String ARG_ADDRESS = "arg_address";
  @NonNull
  private final UGCRatingAdapter mUGCRatingAdapter = new UGCRatingAdapter();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private EditText mReviewEditText;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_editor, container, false);
    mReviewEditText = root.findViewById(R.id.review);

    RecyclerView rvRatingView = root.findViewById(R.id.ratings);
    rvRatingView.setLayoutManager(new LinearLayoutManager(getContext()));
    rvRatingView.getLayoutManager().setAutoMeasureEnabled(true);
    rvRatingView.setNestedScrollingEnabled(false);
    rvRatingView.setHasFixedSize(false);
    rvRatingView.setAdapter(mUGCRatingAdapter);
    Bundle args = getArguments();
    if (args == null)
      throw new AssertionError("Args must be passed to this fragment!");

    UiUtils.showIf(args.getBoolean(ARG_CAN_BE_REVIEWED), mReviewEditText);

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
    getToolbarController().setTitle(getArguments().getString(ARG_TITLE));
    View submitButton = getToolbarController().findViewById(R.id.submit);
    submitButton.setOnClickListener(v ->
                                    {
                                      onSubmitButtonClick();
                                      if (!ConnectionState.isConnected())
                                      {
                                        finishActivity();
                                        return;
                                      }
                                      authorize();
                                    });
  }

  @NonNull
  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(root, getActivity())
    {
      @Override
      public void onUpClick()
      {
        Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_REVIEW_CANCEL);
        super.onUpClick();
      }
    };
  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {
    if (success)
    {
      final Notifier notifier = Notifier.from(getActivity().getApplication());
      notifier.cancelNotification(Notifier.ID_IS_NOT_AUTHENTICATED);
    }

    finishActivity();
  }

  private void finishActivity()
  {
    if (isAdded())
      getActivity().finish();
  }

  @Override
  public void onAuthorizationStart()
  {
    finishActivity();
  }

  @Override
  public void onSocialAuthenticationCancel(@Framework.AuthTokenType int type)
  {
    Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_AUTH_DECLINED);
    finishActivity();
  }

  @Override
  public void onSocialAuthenticationError(int type, @Nullable String error)
  {
    Statistics.INSTANCE.trackUGCAuthFailed(type, error);
    finishActivity();
  }

  private void onSubmitButtonClick()
  {
    List<UGC.Rating> modifiedRatings = mUGCRatingAdapter.getItems();
    UGC.Rating[] ratings = new UGC.Rating[modifiedRatings.size()];
    modifiedRatings.toArray(ratings);
    UGCUpdate update = new UGCUpdate(ratings, mReviewEditText.getText().toString(),
                                     System.currentTimeMillis(), Language.getDefaultLocale(),
                                     Language.getKeyboardLocale());
    FeatureId featureId = getArguments().getParcelable(ARG_FEATURE_ID);
    if (featureId == null)
    {

      throw new AssertionError("Feature ID must be non-null for ugc object! " +
                               "Title = " + getArguments().getString(ARG_TITLE) +
                               "; address = " + getArguments().getString(ARG_ADDRESS) +
                               "; lat = " + getArguments().getDouble(ARG_LAT) +
                               "; lon = " + getArguments().getDouble(ARG_LON));
    }
    UGC.setUGCUpdate(featureId, update);
    UserActionsLogger.logUgcSaved();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_REVIEW_SUCCESS);
  }
}
