package com.mapswithme.maps.ugc.routes;

import android.content.Intent;
import androidx.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public class EditCategoryNameFragment extends BaseEditUserBookmarkCategoryFragment
{
  public static final int REQ_CODE_EDIT_DESCRIPTION = 75;

  @Override
  protected int getHintText()
  {
    return R.string.name_placeholder;
  }

  @Override
  @NonNull
  protected CharSequence getTopSummaryText()
  {
    return getString(R.string.name_comment1);
  }

  @NonNull
  @Override
  protected CharSequence getBottomSummaryText()
  {
    return getString(R.string.name_comment2);
  }

  @Override
  protected void onDoneOptionItemClicked()
  {
    BookmarkManager.INSTANCE.setCategoryName(getCategory().getId(),
                                             getEditText().getText().toString().trim());
    openNextScreen();
  }

  private void openNextScreen()
  {
    Intent intent = new Intent(getContext(), EditCategoryDescriptionActivity.class);
    intent.putExtra(BaseEditUserBookmarkCategoryFragment.BUNDLE_BOOKMARK_CATEGORY, getCategory());
    startActivityForResult(intent, REQ_CODE_EDIT_DESCRIPTION);
  }

  @NonNull
  @Override
  protected CharSequence getEditableText()
  {
    return getCategory().getName();
  }
}
