package com.mapswithme.maps.gallery;

import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import com.mapswithme.maps.base.BaseMwmExtraTitleActivity;

import java.util.ArrayList;

public class GalleryActivity extends BaseMwmExtraTitleActivity
{
  public static final String EXTRA_IMAGES = "gallery_images";

  public static void start(Context context, @NonNull ArrayList<Image> images, @NonNull String title)
  {
    final Intent i = new Intent(context, GalleryActivity.class);
    i.putParcelableArrayListExtra(EXTRA_IMAGES, images);
    i.putExtra(EXTRA_TITLE, title);
    context.startActivity(i);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return GalleryFragment.class;
  }
}
