package com.mapswithme.maps.gallery;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.UiUtils;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.widget.Toolbar;

import java.util.ArrayList;

public class GalleryActivity extends BaseMwmFragmentActivity {
  public static final String EXTRA_IMAGES = "gallery_images";
  public static final String EXTRA_TITLE = "gallery_title";

  public static void start(Context context, ArrayList<Image> images, String title)
  {
    final Intent i = new Intent(context, GalleryActivity.class);
    i.putParcelableArrayListExtra(EXTRA_IMAGES, images);
    i.putExtra(EXTRA_TITLE, title);
    context.startActivity(i);
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    String title = "";
    Bundle bundle = getIntent().getExtras();
    if (bundle != null) {
      title = bundle.getString(EXTRA_TITLE);
    }
    Toolbar toolbar = getToolbar();
    toolbar.setTitle(title);
    UiUtils.showHomeUpButton(toolbar);
    displayToolbarAsActionBar();
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass() {
    return GalleryFragment.class;
  }

  @Override
  protected int getContentLayoutResId() {
    return R.layout.activity_fragment_and_toolbar;
  }

  @Override
  protected int getFragmentContentResId() {
    return R.id.fragment_container;
  }
}
