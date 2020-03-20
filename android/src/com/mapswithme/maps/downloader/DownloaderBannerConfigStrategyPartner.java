package com.mapswithme.maps.downloader;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.ColorInt;
import androidx.annotation.ColorRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

class DownloaderBannerConfigStrategyPartner implements DownloaderBannerConfigStrategy
{
  @DrawableRes
  private final int mIcon;
  @StringRes
  private final int mMessage;
  @StringRes
  private final int mButtonText;
  @ColorRes
  private final int mButtonTextColor;
  @ColorRes
  private final int mButtonColor;

  DownloaderBannerConfigStrategyPartner(@DrawableRes int icon, @StringRes int message,
                                        @StringRes int buttonText, @ColorRes int buttonTextColor,
                                        @ColorRes int buttonColor)
  {
    mIcon = icon;
    mMessage = message;
    mButtonText = buttonText;
    mButtonTextColor = buttonTextColor;
    mButtonColor = buttonColor;
  }

  @Override
  public void configureView(@NonNull View parent, @IdRes int iconViewId, @IdRes int messageViewId,
                            @IdRes int buttonViewId)
  {
    ImageView icon = parent.findViewById(iconViewId);
    icon.setImageResource(mIcon);
    TextView message = parent.findViewById(messageViewId);
    message.setText(mMessage);
    TextView button = parent.findViewById(buttonViewId);
    button.setText(mButtonText);
    button.setTextColor(button.getResources().getColor(mButtonTextColor));
    button.setBackgroundColor(button.getResources().getColor(mButtonColor));
  }
}
