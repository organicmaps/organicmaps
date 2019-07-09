package com.mapswithme.maps.promo;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import com.bumptech.glide.Glide;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;

public class PromoBookingDialogFragment extends BaseMwmDialogFragment
{
  public static final String EXTRA_CITY_GUIDES_URL = "city_guides_url";
  public static final String EXTRA_CITY_IMAGE_URL = "city_image_url";

  @Nullable
  private String mCityGuidesUrl;
  @Nullable
  private String mCityImageUrl;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_promo_after_booking_dialog, container, false);
  }

  @Override
  protected int getStyle()
  {
    return STYLE_NO_TITLE;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    View cancel = view.findViewById(R.id.cancel);
    cancel.setOnClickListener(v -> dismissAllowingStateLoss());

    if (!readArguments())
      return;

    loadImage();

    View cityGuides = view.findViewById(R.id.city_guides);
    cityGuides.setOnClickListener(v -> onCityGuidesClick());
  }

  private boolean readArguments()
  {
    final Bundle arguments = getArguments();

    if (arguments == null)
      return false;

    mCityGuidesUrl = arguments.getString(EXTRA_CITY_GUIDES_URL);
    mCityImageUrl = arguments.getString(EXTRA_CITY_IMAGE_URL);

    return !TextUtils.isEmpty(mCityGuidesUrl) && !TextUtils.isEmpty(mCityImageUrl);
  }

  private void loadImage()
  {
    if (mCityImageUrl == null)
      return;

    ImageView imageView = getViewOrThrow().findViewById(R.id.city_picture);
    Glide.with(imageView.getContext())
         .load(mCityImageUrl)
         .centerCrop()
         .into(imageView);
  }

  private void onCityGuidesClick()
  {
    FragmentActivity activity = getActivity();
    if (activity == null || mCityGuidesUrl == null)
      return;

    BookmarksCatalogActivity.start(activity, mCityGuidesUrl);
    dismissAllowingStateLoss();
  }
}
