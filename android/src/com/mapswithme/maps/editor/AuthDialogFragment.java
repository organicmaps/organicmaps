package com.mapswithme.maps.editor;

import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.statistics.Statistics;

import java.util.Objects;

public class AuthDialogFragment extends BaseMwmDialogFragment
{

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = getArguments();
    if (args == null)
      return;

    sendStats(args, Statistics.EventName.AUTH_SHOWN);
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    return new DialogImpl();
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_auth_editor_dialog, container, false);
  }

  @Override
  protected int getStyle()
  {
    return STYLE_NO_TITLE;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    OsmAuthFragmentDelegate osmAuthDelegate = new AuthFragmentDelegate();
    osmAuthDelegate.onViewCreated(view, savedInstanceState);
  }

  private static void sendStats(@NonNull Bundle args, @NonNull String action)
  {
    Statistics.INSTANCE.trackAuthDialogAction(action,
                                              Objects.requireNonNull(args.getString(Statistics.EventParam.FROM)));
  }

  private class AuthFragmentDelegate extends OsmAuthFragmentDelegate
  {
    AuthFragmentDelegate()
    {
      super(AuthDialogFragment.this);
    }

    @Override
    protected void loginOsm()
    {
      startActivity(new Intent(getContext(), OsmAuthActivity.class));
      dismiss();
      if (getArguments() == null)
        return;

      sendStats(getArguments(), Statistics.EventName.AUTH_START);
    }
  }

  private class DialogImpl extends Dialog
  {
    DialogImpl()
    {
      super(requireActivity(), getTheme());
    }

    @Override
    public void onBackPressed()
    {
      super.onBackPressed();
      if (getArguments() == null)
        return;

      sendStats(getArguments(), Statistics.EventName.AUTH_DECLINED);
    }
  }
}
