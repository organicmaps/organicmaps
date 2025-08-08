package app.organicmaps.sdk.util;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentManager;
import java.util.concurrent.TimeUnit;

@Keep
public final class NetworkPolicy
{
  public enum Type
  {
    ASK,

    ALWAYS() {
      @Override
      public void check(@NonNull DialogPresenter dialogPresenter, @NonNull FragmentManager fragmentManager,
                        @NonNull NetworkPolicyListener listener, boolean isDialogAllowed)
      {
        boolean nowInRoaming = ConnectionState.INSTANCE.isInRoaming();
        boolean acceptedInRoaming = Config.getMobileDataRoaming();

        if (nowInRoaming && !acceptedInRoaming)
          dialogPresenter.showDialog(fragmentManager, listener);
        else
          listener.onResult(new NetworkPolicy(true));
      }
    },

    NEVER() {
      @Override
      public void check(@NonNull DialogPresenter dialogPresenter, @NonNull FragmentManager fragmentManager,
                        @NonNull NetworkPolicyListener listener, boolean isDialogAllowed)
      {
        if (isDialogAllowed)
          dialogPresenter.showDialog(fragmentManager, listener);
        else
          listener.onResult(new NetworkPolicy(false));
      }
    },

    NOT_TODAY() {
      @Override
      public void check(@NonNull DialogPresenter dialogPresenter, @NonNull FragmentManager fragmentManager,
                        @NonNull NetworkPolicyListener listener, boolean isDialogAllowed)
      {
        if (isDialogAllowed)
          dialogPresenter.showDialog(fragmentManager, listener);
        else
          dialogPresenter.showDialogIfNeeded(fragmentManager, listener, new NetworkPolicy(false), isToday());
      }
    },

    TODAY() {
      @Override
      public void check(@NonNull DialogPresenter dialogPresenter, @NonNull FragmentManager fragmentManager,
                        @NonNull NetworkPolicyListener listener, boolean isDialogAllowed)
      {
        boolean nowInRoaming = ConnectionState.INSTANCE.isInRoaming();
        boolean acceptedInRoaming = Config.getMobileDataRoaming();

        if (nowInRoaming && !acceptedInRoaming)
          dialogPresenter.showDialog(fragmentManager, listener);
        else
          dialogPresenter.showDialogIfNeeded(fragmentManager, listener, new NetworkPolicy(true), isToday());
      }
    };

    public void check(@NonNull DialogPresenter dialogPresenter, @NonNull FragmentManager fragmentManager,
                      @NonNull final NetworkPolicyListener listener, boolean isDialogAllowed)
    {
      dialogPresenter.showDialog(fragmentManager, listener);
    }
  }

  public static final int NONE = -1;

  public static void checkNetworkPolicy(@NonNull DialogPresenter dialogPresenter,
                                        @NonNull FragmentManager fragmentManager,
                                        @NonNull final NetworkPolicyListener listener, boolean isDialogAllowed)
  {
    if (ConnectionState.INSTANCE.isWifiConnected())
    {
      listener.onResult(new NetworkPolicy(true));
      return;
    }

    if (!ConnectionState.INSTANCE.isMobileConnected())
    {
      listener.onResult(new NetworkPolicy(false));
      return;
    }

    Type type = Config.getUseMobileDataSettings();
    type.check(dialogPresenter, fragmentManager, listener, isDialogAllowed);
  }

  public static void checkNetworkPolicy(@NonNull DialogPresenter dialogPresenter,
                                        @NonNull FragmentManager fragmentManager,
                                        @NonNull final NetworkPolicyListener listener)
  {
    checkNetworkPolicy(dialogPresenter, fragmentManager, listener, false);
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public static boolean getCurrentNetworkUsageStatus()
  {
    if (ConnectionState.INSTANCE.isWifiConnected())
      return true;

    if (!ConnectionState.INSTANCE.isMobileConnected())
      return false;

    boolean nowInRoaming = ConnectionState.INSTANCE.isInRoaming();
    boolean acceptedInRoaming = Config.getMobileDataRoaming();
    if (nowInRoaming && !acceptedInRoaming)
      return false;

    Type type = Config.getUseMobileDataSettings();
    return type == Type.ALWAYS || (type == Type.TODAY && isToday());
  }

  private static boolean isToday()
  {
    long timestamp = Config.getMobileDataTimeStamp();
    return TimeUnit.MILLISECONDS.toDays(System.currentTimeMillis() - timestamp) < 1;
  }

  private final boolean mCanUseNetwork;

  public NetworkPolicy(boolean canUse)
  {
    mCanUseNetwork = canUse;
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public boolean canUseNetwork()
  {
    return mCanUseNetwork;
  }

  public interface NetworkPolicyListener
  {
    void onResult(@NonNull NetworkPolicy policy);
  }

  public interface DialogPresenter
  {
    void showDialogIfNeeded(@NonNull FragmentManager fragmentManager,
                            @NonNull NetworkPolicy.NetworkPolicyListener listener, @NonNull NetworkPolicy policy,
                            boolean isToday);

    void showDialog(@NonNull FragmentManager fragmentManager, @NonNull NetworkPolicy.NetworkPolicyListener listener);
  }
}
