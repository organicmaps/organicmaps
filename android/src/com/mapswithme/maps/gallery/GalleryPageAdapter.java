package com.mapswithme.maps.gallery;

import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;

import java.util.List;

class GalleryPageAdapter extends FragmentStatePagerAdapter
{

  @NonNull
  private final List<Image> mImages;

  GalleryPageAdapter(@NonNull FragmentManager fm, @NonNull List<Image> images)
  {
    super(fm);
    mImages = images;
  }

  @Override
  public Fragment getItem(int position)
  {
    Bundle args = new Bundle();
    args.putParcelable(FullScreenGalleryFragment.ARGUMENT_IMAGE, mImages.get(position));
    FullScreenGalleryFragment fragment = new FullScreenGalleryFragment();
    fragment.setArguments(args);
    return fragment;
  }

  @Override
  public int getCount()
  {
    return mImages.size();
  }

  @NonNull
  Image getImage(int position)
  {
    return mImages.get(position);
  }
}
