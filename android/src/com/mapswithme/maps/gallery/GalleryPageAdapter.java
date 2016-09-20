package com.mapswithme.maps.gallery;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;

import java.util.List;

class GalleryPageAdapter extends FragmentStatePagerAdapter {

  private final List<Image> mImages;

  GalleryPageAdapter(FragmentManager fm, List<Image> images) {
    super(fm);
    mImages = images;
  }

  @Override
  public Fragment getItem(int position) {
    Bundle args = new Bundle();
    args.putParcelable(FullScreenGalleryFragment.ARGUMENT_IMAGE, mImages.get(position));
    FullScreenGalleryFragment fragment = new FullScreenGalleryFragment();
    fragment.setArguments(args);
    return fragment;
  }

  @Override
  public int getCount() {
    return mImages.size();
  }

  Image getImage(int position) {
    return mImages.get(position);
  }
}
