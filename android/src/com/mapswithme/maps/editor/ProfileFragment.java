package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.text.format.DateUtils;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.Locale;

import com.mapswithme.maps.R;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.UiUtils;

public class ProfileFragment extends AuthFragment implements View.OnClickListener
{
  private View mEditsBlock;
  private TextView mEditsLocal;
  private View mEditsMore;
  private TextView mEditsSent;
  private TextView mEditsSentDate;
  private View mLogout;
  private View mAuthBlock;

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController.setTitle(R.string.profile);
    initViews(view);
    refreshViews();
  }

  private void initViews(View view)
  {
    mLogout = mToolbarController.findViewById(R.id.logout);
    mLogout.setOnClickListener(this);
    mEditsBlock = view.findViewById(R.id.block_edits);
    UiUtils.show(mEditsBlock);
    final View localEdits = mEditsBlock.findViewById(R.id.local_edits);
    ((ImageView) localEdits.findViewById(R.id.image)).setImageResource(R.drawable.ic_device);
    mEditsLocal = (TextView) localEdits.findViewById(R.id.title);
    mEditsMore = localEdits.findViewById(R.id.more);
    mEditsMore.setOnClickListener(this);
    UiUtils.hide(localEdits.findViewById(R.id.subtitle), localEdits.findViewById(R.id.more));
    final View sentEdits = mEditsBlock.findViewById(R.id.sent_edits);
    ((ImageView) sentEdits.findViewById(R.id.image)).setImageResource(R.drawable.ic_upload);
    mEditsSent = (TextView) sentEdits.findViewById(R.id.title);
    mEditsSentDate = (TextView) sentEdits.findViewById(R.id.subtitle);
    UiUtils.hide(sentEdits.findViewById(R.id.more));
    mAuthBlock = view.findViewById(R.id.block_auth);
    ((TextView) mAuthBlock.findViewById(R.id.first_osm_edit)).setText(R.string.login_and_edit_map_motivation_message);
  }

  private void refreshViews()
  {
    if (OsmOAuth.isAuthorized())
    {
      UiUtils.show(mLogout);
      UiUtils.hide(mAuthBlock);
    }
    else
    {
      UiUtils.show(mAuthBlock);
      UiUtils.hide(mLogout);
    }
    final long[] stats = Editor.nativeGetStats();
    mEditsLocal.setText(String.format(Locale.US, "%s %d", getString(R.string.editor_profile_unsent_changes), stats[0] - stats[1]));
    mEditsSent.setText(String.format(Locale.US, "%s %d", getString(R.string.editor_profile_changes), + stats[1]));
    if (stats[1] == 0)
      UiUtils.hide(mEditsSentDate);
    else
      UiUtils.setTextAndShow(mEditsSentDate, "Upload date : " + DateUtils.formatDateTime(getActivity(), stats[2] * 1000, 0));

    UiUtils.showIf(stats[1] != stats[0], mEditsMore);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.logout:
      OsmOAuth.clearAuthorization();
      refreshViews();
      break;
    case R.id.more:
      BottomSheetHelper.create(getActivity())
                       .sheet(Menu.NONE, R.drawable.ic_delete, R.string.delete)
                       .listener(new MenuItem.OnMenuItemClickListener()
                       {
                         @Override
                         public boolean onMenuItemClick(MenuItem menuItem)
                         {
                           Editor.nativeClearLocalEdits();
                           refreshViews();
                           return false;
                         }
                       }).tint().show();
      break;
    default:
      super.onClick(v);
    }
  }
}
