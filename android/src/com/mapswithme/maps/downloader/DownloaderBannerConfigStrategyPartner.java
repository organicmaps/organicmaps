package com.mapswithme.maps.downloader;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.ColorInt;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

class DownloaderBannerConfigStrategyPartner implements DownloaderBannerConfigStrategy
{
  @DrawableRes
  private int mIcon;
  @StringRes
  private int mMessage;
  @StringRes
  private int mButtonText;
  @ColorInt
  private int mButtonTextColor;
  @ColorInt
  private int mButtonColor;

  DownloaderBannerConfigStrategyPartner(@DrawableRes int icon, @StringRes int message,
                                        @StringRes int buttonText, @ColorInt int buttonTextColor,
                                        @ColorInt int buttonColor)
  {
    mIcon = icon;
    mMessage = message;
    mButtonText = buttonText;
    mButtonTextColor = buttonTextColor;
    mButtonColor = buttonColor;
  }

  @Override
  public void ConfigureView(@NonNull View parent, @IdRes int iconViewId, @IdRes int messageViewId,
                            @IdRes int buttonViewId)
  {
    ImageView icon = parent.findViewById(iconViewId);
    icon.setImageResource(mIcon);
    TextView message = parent.findViewById(messageViewId);
    message.setText(mMessage);
    TextView button = parent.findViewById(buttonViewId);
    button.setText(mButtonText);
    button.setTextColor(mButtonTextColor);
    button.setBackgroundColor(mButtonColor);
  }
}
