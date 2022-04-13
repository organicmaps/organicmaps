package com.mapswithme.maps.editor;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;

import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.bottomsheet.MenuBottomSheetFragment;
import com.mapswithme.util.bottomsheet.MenuBottomSheetItem;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;

import java.util.ArrayList;

public class ProfileFragment extends AuthFragment implements View.OnClickListener
{
  private View mSentBlock;
  private TextView mEditsSent;
  private View mMore;
  private View mAuthBlock;
  private View mRatingBlock;

  private void onLogoutActionSelected(final ProfileFragment fragment)
  {
    new AlertDialog.Builder(fragment.requireContext())
        .setMessage(R.string.are_you_sure)
        .setPositiveButton(android.R.string.ok, (dialog, which) ->
        {
          OsmOAuth.clearAuthorization(fragment.requireContext());
          fragment.refreshViews();
        })
        .setNegativeButton(android.R.string.no, null)
        .create()
        .show();
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
        final int count = OsmOAuth.getOsmChangesetsCount(requireContext(), getParentFragmentManager());
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
      startActivity(new Intent((Intent.ACTION_VIEW), Uri.parse(OsmOAuth.getHistoryUrl(requireContext()))));
      break;
    }
  }

  private void showBottomSheet()
  {
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    items.add(new MenuBottomSheetItem(R.string.logout, R.drawable.ic_logout, () -> onLogoutActionSelected(ProfileFragment.this)));
    new MenuBottomSheetFragment(items).show(getParentFragmentManager(), "profileBottomSheet");
  }
}
