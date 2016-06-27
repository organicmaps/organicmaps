package com.mapswithme.maps.editor;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.text.format.DateUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import com.mapswithme.maps.R;
import com.mapswithme.maps.editor.data.UserStats;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;

public class ProfileFragment extends AuthFragment implements View.OnClickListener, OsmOAuth.OnUserStatsChanged
{
  private View mLocalBlock;
  private TextView mEditsLocal;
  private View mSentBlock;
  private TextView mEditsSent;
  private TextView mEditsSentDate;
  private View mMore;
  private View mAuthBlock;
  private View mRatingBlock;
  private TextView mEditorRank;
  private TextView mEditorLevelUp;

  private enum MenuItem
  {
    LOGOUT(R.drawable.ic_logout, R.string.logout)
    {
      @Override
      void invoke(ProfileFragment fragment)
      {
        OsmOAuth.clearAuthorization();
        fragment.refreshViews();
      }
    },

    REFRESH(R.drawable.ic_update, R.string.refresh)
    {
      @Override
      void invoke(ProfileFragment fragment)
      {
        OsmOAuth.nativeUpdateOsmUserStats(OsmOAuth.getUsername(), true /* forceUpdate */);
      }
    };

    final @DrawableRes int icon;
    final @StringRes int title;

    MenuItem(@DrawableRes int icon, @StringRes int title)
    {
      this.icon = icon;
      this.title = title;
    }

    abstract void invoke(ProfileFragment fragment);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController.setTitle(R.string.profile);
    initViews(view);
    refreshViews();
    OsmOAuth.setUserStatsListener(this);
    OsmOAuth.nativeUpdateOsmUserStats(OsmOAuth.getUsername(), false /* forceUpdate */);
  }

  private void initViews(View view)
  {
    mMore = mToolbarController.findViewById(R.id.more);
    mMore.setOnClickListener(this);
    View editsBlock = view.findViewById(R.id.block_edits);
    UiUtils.show(editsBlock);
    mLocalBlock = editsBlock.findViewById(R.id.local_edits);
    ((ImageView) mLocalBlock.findViewById(R.id.image)).setImageResource(R.drawable.ic_device);
    mEditsLocal = (TextView) mLocalBlock.findViewById(R.id.title);
    mSentBlock = editsBlock.findViewById(R.id.sent_edits);
    mEditsSent = (TextView) mSentBlock.findViewById(R.id.edits_count);
    mEditsSentDate = (TextView) mSentBlock.findViewById(R.id.date_sent);
    mAuthBlock = view.findViewById(R.id.block_auth);
    mRatingBlock = view.findViewById(R.id.block_rating);
    mEditorRank = (TextView) mRatingBlock.findViewById(R.id.rating);
    // FIXME show when it will be implemented on server
//    mEditorLevelUp = mRatingBlock.findViewById(R.id.level_up_feat);
    view.findViewById(R.id.about_osm).setOnClickListener(this);
  }

  private void refreshViews()
  {
    if (OsmOAuth.isAuthorized())
    {
      UiUtils.show(mMore, mRatingBlock, mSentBlock);
      UiUtils.hide(mAuthBlock);
    }
    else
    {
      UiUtils.show(mAuthBlock);
      UiUtils.hide(mMore, mRatingBlock, mSentBlock);
    }

    final long[] stats = Editor.nativeGetStats();
    final long localCount = stats[0] - stats[1];
    if (localCount == 0)
    {
      UiUtils.hide(mLocalBlock);
    }
    else
    {
      UiUtils.show(mLocalBlock);
      mEditsLocal.setText(String.format(Locale.US, "%s %d", getString(R.string.editor_profile_unsent_changes), localCount));
    }

    refreshRatings(0, 0, 0, "");
  }

  private void refreshRatings(long uploadedCount, long uploadSeconds, long rank, String levelFeat)
  {
    String edits, editsDate;

    if (uploadedCount == 0)
    {
      edits = editsDate = "---";
    }
    else
    {
      edits = String.valueOf(uploadedCount);
      editsDate = DateUtils.formatDateTime(getActivity(), uploadSeconds * 1000, DateUtils.FORMAT_SHOW_DATE | DateUtils.FORMAT_SHOW_TIME);
    }
    mEditsSent.setText(edits);
    mEditsSentDate.setText(getString(R.string.last_update, editsDate));
    mEditorRank.setText(String.valueOf(rank));
    // FIXME show when it will be implemented on server
//    mEditorLevelUp.setText(levelFeat);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.more:
      showBottomSheet();
      break;
    case R.id.about_osm:
      startActivity(new Intent((Intent.ACTION_VIEW), Uri.parse(Constants.Url.OSM_ABOUT)));
      break;
    }
  }

  @Override
  public void onStatsChange(final UserStats stats)
  {
    if (!isAdded())
      return;

    if (stats == null)
      refreshRatings(0, 0, 0, "");
    else
      refreshRatings(stats.editsCount, stats.uploadTimestampSeconds, stats.editorRank, stats.levelUp);
  }

  private void showBottomSheet()
  {
    List<MenuItem> items = new ArrayList<>();
    items.add(MenuItem.REFRESH);
    items.add(MenuItem.LOGOUT);

    BottomSheetHelper.Builder bs = BottomSheetHelper.create(getActivity());
    for (MenuItem item: items)
      bs.sheet(item.ordinal(), item.icon, item.title);

    bs.listener(new android.view.MenuItem.OnMenuItemClickListener()
    {
      @Override
      public boolean onMenuItemClick(android.view.MenuItem item)
      {
        MenuItem.values()[item.getItemId()].invoke(ProfileFragment.this);
        return false;
      }
    }).tint().show();
  }
}
