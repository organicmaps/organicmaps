package com.mapswithme.maps.editor;

import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AlertDialog;

import com.cocosw.bottomsheet.BottomSheet;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;

import java.util.ArrayList;
import java.util.List;

public class ProfileFragment extends AuthFragment implements View.OnClickListener
{
  private View mSentBlock;
  private TextView mEditsSent;
  private View mMore;
  private View mAuthBlock;
  private View mRatingBlock;

  private enum MenuItem
  {
    LOGOUT(R.drawable.ic_logout, R.string.logout)
    {
      @Override
      void invoke(final ProfileFragment fragment)
      {
        new AlertDialog.Builder(fragment.requireContext())
            .setMessage(R.string.are_you_sure)
            .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener()
            {
              @Override
              public void onClick(DialogInterface dialog, int which)
              {
                OsmOAuth.clearAuthorization(fragment.requireContext());
                fragment.refreshViews();
              }
            })
            .setNegativeButton(android.R.string.no, null)
            .create()
            .show();
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
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.profile);
    initViews(view);
    refreshViews();
  }

  private void initViews(View view)
  {
    mMore = getToolbarController().getToolbar().findViewById(R.id.more);
    mMore.setOnClickListener(this);
    View editsBlock = view.findViewById(R.id.block_edits);
    UiUtils.show(editsBlock);
    mSentBlock = editsBlock.findViewById(R.id.sent_edits);
    mEditsSent = mSentBlock.findViewById(R.id.edits_count);
    mAuthBlock = view.findViewById(R.id.block_auth);
    mRatingBlock = view.findViewById(R.id.block_rating);
    view.findViewById(R.id.about_osm).setOnClickListener(this);
    view.findViewById(R.id.osm_history).setOnClickListener(this);
  }

  private void refreshViews()
  {
    if (OsmOAuth.isAuthorized(requireContext()))
    {
      UiUtils.show(mMore, mRatingBlock, mSentBlock);
      UiUtils.hide(mAuthBlock);
      // Update the number of uploaded changesets from OSM.
      ThreadPool.getWorker().execute(() -> {
        final int count = OsmOAuth.getOsmChangesetsCount(requireContext());
        UiThread.run(() -> mEditsSent.setText(String.valueOf(count)));
      });
    }
    else
    {
      UiUtils.show(mAuthBlock);
      UiUtils.hide(mMore, mRatingBlock, mSentBlock);
    }
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
    case R.id.osm_history:
      // Debug builds use dev OSM playground for APIs.
      final String url = BuildConfig.DEBUG
          ? "https://master.apis.dev.openstreetmap.org/user/%s/history"
          : "https://www.openstreetmap.org/user/%s/history";
      startActivity(new Intent((Intent.ACTION_VIEW),
          Uri.parse(String.format(url, OsmOAuth.getUsername(requireContext())))));
      break;
    }
  }

  private void showBottomSheet()
  {
    List<MenuItem> items = new ArrayList<>();
    items.add(MenuItem.LOGOUT);

    BottomSheetHelper.Builder bs = BottomSheetHelper.create(getActivity());
    for (MenuItem item: items)
      bs.sheet(item.ordinal(), item.icon, item.title);

    BottomSheet bottomSheet = bs.listener(new android.view.MenuItem.OnMenuItemClickListener()
    {
      @Override
      public boolean onMenuItemClick(android.view.MenuItem item)
      {
        MenuItem.values()[item.getItemId()].invoke(ProfileFragment.this);
        return false;
      }
    }).build();
    BottomSheetHelper.tint(bottomSheet);
    bottomSheet.show();
  }
}
