package com.mapswithme.maps.gallery;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.BitmapImageViewTarget;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawableFactory;
import android.support.v4.view.ViewPager;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class FullScreenGalleryActivity extends BaseMwmFragmentActivity
        implements ViewPager.OnPageChangeListener {
  public static final String EXTRA_IMAGES = "gallery_images";
  public static final String EXTRA_POSITION = "gallery_position";

  private List<Image> mImages;
  private int mPosition;
  private View mUserBlock;
  private TextView mDescription;
  private TextView mUserName;
  private TextView mSource;
  private TextView mDate;
  private ImageView mAvatar;

  private GalleryPageAdapter mGalleryPageAdapter;

  public static void start(Context context, ArrayList<Image> images, int position)
  {
    final Intent i = new Intent(context, FullScreenGalleryActivity.class);
    i.putParcelableArrayListExtra(EXTRA_IMAGES, images);
    i.putExtra(EXTRA_POSITION, position);
    context.startActivity(i);
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    requestWindowFeature(Window.FEATURE_NO_TITLE);
    getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

    super.onCreate(savedInstanceState);
    Toolbar toolbar = getToolbar();
    toolbar.setTitle("");
    UiUtils.showHomeUpButton(toolbar);
    displayToolbarAsActionBar();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
    }

    mUserBlock = findViewById(R.id.rl__user_block);
    mDescription = (TextView) findViewById(R.id.tv__description);
    mUserName = (TextView) findViewById(R.id.tv__name);
    mSource = (TextView) findViewById(R.id.tv__source);
    mDate = (TextView) findViewById(R.id.tv__date);
    mAvatar = (ImageView) findViewById(R.id.iv__avatar);

    readParameters();
    mGalleryPageAdapter = new GalleryPageAdapter(getSupportFragmentManager(), mImages);
    ViewPager viewPager = (ViewPager) findViewById(R.id.vp__image);
    viewPager.setAdapter(mGalleryPageAdapter);
    viewPager.addOnPageChangeListener(this);
    viewPager.setCurrentItem(mPosition);
  }

  @Override
  public int getThemeResourceId(String theme)
  {
    if (ThemeUtils.isDefaultTheme(theme))
      return R.style.MwmTheme_FullScreenGalleryActivity;

    if (ThemeUtils.isNightTheme(theme))
      return R.style.MwmTheme_Night_FullScreenGalleryActivity;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }


  @Override
  protected int getContentLayoutResId() {
    return R.layout.activity_viewpager_transparent_toolbar;
  }

  @Override
  public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {

  }

  @Override
  public void onPageSelected(int position) {
    updateInformation(mGalleryPageAdapter.getImage(position));
  }

  @Override
  public void onPageScrollStateChanged(int state) {

  }

  private void readParameters() {
    Bundle extras = getIntent().getExtras();
    if (extras != null) {
      mImages = extras.getParcelableArrayList(EXTRA_IMAGES);
      mPosition = extras.getInt(EXTRA_POSITION);
    }
  }

  private void updateInformation(Image image) {
    UiUtils.setTextAndHideIfEmpty(mDescription, image.getDescription());
    UiUtils.setTextAndHideIfEmpty(mUserName, image.getUserName());
    UiUtils.setTextAndHideIfEmpty(mSource, image.getSource());
    if (image.getDate() != null) {
      Date date = new Date(image.getDate());
      mDate.setText(DateFormat.getMediumDateFormat(this).format(date));
      UiUtils.show(mDate);
    } else {
      UiUtils.hide(mDate);
    }
    if (!TextUtils.isEmpty(image.getUserAvatar())) {
      UiUtils.show(mAvatar);
      Glide.with(this)
              .load(image.getUserAvatar())
              .asBitmap()
              .centerCrop()
              .into(new BitmapImageViewTarget(mAvatar) {
                @Override
                protected void setResource(Bitmap resource) {
                  RoundedBitmapDrawable circularBitmapDrawable =
                          RoundedBitmapDrawableFactory.create(getResources(), resource);
                  circularBitmapDrawable.setCircular(true);
                  mAvatar.setImageDrawable(circularBitmapDrawable);
                }
              });
    } else {
      UiUtils.hide(mAvatar);
    }
    if (UiUtils.isHidden(mUserName)
            && UiUtils.isHidden(mSource)
            && UiUtils.isHidden(mDate)
            && UiUtils.isHidden(mAvatar)) {
      UiUtils.hide(mUserBlock);
    } else {
      UiUtils.show(mUserBlock);
    }
  }
}
