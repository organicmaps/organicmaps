package com.mapswithme.maps.gallery;

import com.bumptech.glide.Glide;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

public class FullScreenGalleryFragment extends BaseMwmFragment {
  static final String ARGUMENT_IMAGE = "argument_image";

  private Image mImage;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
          @Nullable Bundle savedInstanceState) {
    return inflater.inflate(R.layout.fragment_fullscreen_image, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    readArguments();

    if (mImage != null) {
      ImageView imageView = (ImageView) view.findViewById(R.id.iv__image);
      Glide.with(view.getContext())
              .load(mImage.getUrl())
              .into(imageView);
    }
  }

  private void readArguments() {
    Bundle args = getArguments();
    if (args != null) {
      mImage = args.getParcelable(ARGUMENT_IMAGE);
    }
  }
}
