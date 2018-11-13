package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogCustomProperty;
import com.mapswithme.maps.bookmarks.data.CatalogPropertyOptionAndKey;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class UgcRoutePropertiesFragment extends BaseMwmFragment implements BookmarkManager.BookmarksCatalogListener
{
  public static final String EXTRA_TAGS_ACTIVITY_RESULT = "tags_activity_result";
  public static final String EXTRA_CATEGORY_OPTIONS = "category_options";

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_routes_properties, container, false);
    BookmarkManager.INSTANCE.requestCustomProperties();
    return root;
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addCatalogListener(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeCatalogListener(this);
  }

  @Override
  public void onImportStarted(@NonNull String serverId)
  {

  }

  @Override
  public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
  {

  }

  @Override
  public void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups)
  {

  }

  @Override
  public void onCustomPropertiesReceived(boolean successful,
                                         @NonNull List<CatalogCustomProperty> properties)
  {

  }

  @Override
  public void onUploadStarted(long originCategoryId)
  {

  }

  @Override
  public void onUploadFinished(@NonNull BookmarkManager.UploadResult uploadResult,
                               @NonNull String description, long originCategoryId,
                               long resultCategoryId)
  {

  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (requestCode == UgcSharingOptionsFragment.REQ_CODE_TAGS_ACTIVITY && resultCode == Activity.RESULT_OK)
    {
      Intent intent = new Intent().putExtra(EXTRA_TAGS_ACTIVITY_RESULT, data);
      ArrayList<CatalogPropertyOptionAndKey> options = new ArrayList<>(getSelectedOptions());
      intent.putParcelableArrayListExtra(EXTRA_CATEGORY_OPTIONS, options);
      getActivity().setResult(Activity.RESULT_OK, intent);
      getActivity().finish();
    }
  }

  @NonNull
  private List<CatalogPropertyOptionAndKey> getSelectedOptions()
  {
    /* FIXME later */
    return Collections.emptyList();
  }
}
