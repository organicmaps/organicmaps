package com.mapswithme.maps.gallery;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.StyleRes;
import androidx.core.graphics.drawable.RoundedBitmapDrawable;
import androidx.core.graphics.drawable.RoundedBitmapDrawableFactory;
import androidx.viewpager.widget.ViewPager;
import androidx.appcompat.widget.Toolbar;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.BitmapImageViewTarget;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class FullScreenGalleryActivity extends BaseMwmFragmentActivity
    implements ViewPager.OnPageChangeListener
{
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
  protected void onSafeCreate(Bundle savedInstanceState)
  {
    requestWindowFeature(Window.FEATURE_NO_TITLE);
    getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

    super.onSafeCreate(savedInstanceState);
    Toolbar toolbar = getToolbar();
    toolbar.setTitle("");
    UiUtils.showHomeUpButton(toolbar);
    displayToolbarAsActionBar();

    mUserBlock = findViewById(R.id.rl__user_block);
    mDescription = (TextView) findViewById(R.id.tv__description);
    mUserName = (TextView) findViewById(R.id.tv__name);
    mSource = (TextView) findViewById(R.id.tv__source);
    mDate = (TextView) findViewById(R.id.tv__date);
    mAvatar = (ImageView) findViewById(R.id.iv__avatar);

    readParameters();
    if (mImages != null)
    {
      mGalleryPageAdapter = new GalleryPageAdapter(getSupportFragmentManager(), mImages);
      final ViewPager viewPager = (ViewPager) findViewById(R.id.vp__image);
      viewPager.addOnPageChangeListener(this);
      viewPager.setAdapter(mGalleryPageAdapter);
      viewPager.setCurrentItem(mPosition);
      viewPager.post(new Runnable()
      {
        @Override
        public void run()
        {
          onPageSelected(viewPager.getCurrentItem());
        }
      });
    }
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    Context context = getApplicationContext();

    if (ThemeUtils.isDefaultTheme(context, theme))
      return R.style.MwmTheme_FullScreenGalleryActivity;

    if (ThemeUtils.isNightTheme(context, theme))
      return R.style.MwmTheme_Night_FullScreenGalleryActivity;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.activity_full_screen_gallery;
  }

  @Override
  public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels)
  {
  }

  @Override
  public void onPageSelected(int position)
  {
    updateInformation(mGalleryPageAdapter.getImage(position));
  }

  @Override
  public void onPageScrollStateChanged(int state)
  {
  }

  private void readParameters()
  {
    Bundle extras = getIntent().getExtras();
    if (extras != null)
    {
      mImages = extras.getParcelableArrayList(EXTRA_IMAGES);
      mPosition = extras.getInt(EXTRA_POSITION);
    }
  }

  private void updateInformation(@NonNull Image image)
  {
    UiUtils.setTextAndHideIfEmpty(mDescription, image.getDescription());
    UiUtils.setTextAndHideIfEmpty(mUserName, image.getUserName());
    UiUtils.setTextAndHideIfEmpty(mSource, image.getSource());
    updateDate(image);
    updateUserAvatar(image);
    updateUserBlock();
  }

  private void updateDate(Image image)
  {
    if (image.getDate() != null)
    {
      Date date = new Date(image.getDate());
      mDate.setText(DateFormat.getMediumDateFormat(this).format(date));
      UiUtils.show(mDate);
    }
    else
    {
      UiUtils.hide(mDate);
    }
  }

  private void updateUserAvatar(Image image)
  {
    if (!TextUtils.isEmpty(image.getUserAvatar()))
    {
      UiUtils.show(mAvatar);
      Glide.with(this)
           .load(image.getUserAvatar())
           .asBitmap()
           .centerCrop()
           .into(new BitmapImageViewTarget(mAvatar)
           {
             @Override
             protected void setResource(Bitmap resource)
             {
               RoundedBitmapDrawable circularBitmapDrawable =
                   RoundedBitmapDrawableFactory.create(getResources(), resource);
               circularBitmapDrawable.setCircular(true);
               mAvatar.setImageDrawable(circularBitmapDrawable);
             }
           });
    }
    else
      UiUtils.hide(mAvatar);
  }

  private void updateUserBlock()
  {
    if (UiUtils.isHidden(mUserName)
        && UiUtils.isHidden(mSource)
        && UiUtils.isHidden(mDate)
        && UiUtils.isHidden(mAvatar))
    {
      UiUtils.hide(mUserBlock);
    }
    else
    {
      UiUtils.show(mUserBlock);
    }
  }
}
