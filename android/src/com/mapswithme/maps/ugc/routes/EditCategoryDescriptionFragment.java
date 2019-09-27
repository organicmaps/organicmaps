package com.mapswithme.maps.ugc.routes;

import android.content.Intent;
import androidx.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public class EditCategoryDescriptionFragment extends BaseEditUserBookmarkCategoryFragment
{
  public static final int REQUEST_CODE_CUSTOM_PROPS = 100;
  private static final int TEXT_LIMIT = 500;

  @Override
  protected int getDefaultTextLengthLimit()
  {
    return TEXT_LIMIT;
  }

  @NonNull
  @Override
  protected CharSequence getTopSummaryText()
  {
    return getString(R.string.description_comment1);
  }

  @NonNull
  @Override
  protected CharSequence getBottomSummaryText()
  {
    return "";
  }

  @Override
  protected int getHintText()
  {
    return R.string.description_placeholder;
  }

  @Override
  protected void onDoneOptionItemClicked()
  {
    BookmarkManager.INSTANCE.setCategoryDescription(getCategory().getId(),
                                                    getEditText().getText().toString().trim());
    Intent intent = new Intent(getContext(), UgcRoutePropertiesActivity.class);
    startActivityForResult(intent, REQUEST_CODE_CUSTOM_PROPS);
  }

  @NonNull
  @Override
  protected CharSequence getEditableText()
  {
    return getCategory().getDescription();
  }
}
