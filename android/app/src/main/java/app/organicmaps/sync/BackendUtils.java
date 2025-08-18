package app.organicmaps.sync;

import android.content.Context;
import androidx.annotation.DrawableRes;
import androidx.annotation.StringRes;
import app.organicmaps.sdk.sync.BackendType;

public class BackendUtils
{
  @StringRes
  public static int getDisplayName(BackendType backendType)
  {
    // TODO (PR #10651)
    //   unreachable as of now
    throw new RuntimeException("Not implemented");
  }

  @DrawableRes
  public static int getIcon(BackendType backendType)
  {
    // TODO (PR #10651)
    //   unreachable as of now
    throw new RuntimeException("Not implemented");
  }

  public static void login(Context context, BackendType backendType)
  {
    // noinspection SwitchStatementWithTooFewBranches Will be expanded later with more cloud backends
    switch (backendType)
    {
    // TODO (PR #10651)
    //   unreachable as of now
    default -> throw new RuntimeException("Not implemented");
    }
  }
}
