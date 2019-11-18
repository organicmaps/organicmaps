package com.mapswithme.maps.onboarding;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;

public class OpenCatalogLauncher implements OnboardingScreenLauncher
{
  @Override
  public void launchScreen(@NonNull FragmentActivity activity, @NonNull String url)
  {
    BookmarksCatalogActivity.start(activity, url);
  }
}
