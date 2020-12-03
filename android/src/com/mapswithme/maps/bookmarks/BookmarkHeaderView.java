package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.bumptech.glide.Glide;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.UiUtils;

public class BookmarkHeaderView extends LinearLayout
{

  public static final String AUTHOR_LONELY_PLANET_ID = "28035594-6457-466d-8f6f-8499607df570";

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ImageView mImageView;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private  TextView mTitle;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private TextView mDescriptionBtn;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ImageView mImageViewLogo;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private TextView mAuthorTextView;

  public BookmarkHeaderView(@NonNull Context context)
  {
    super(context);
    init();
  }

  public BookmarkHeaderView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    init();
  }

  public BookmarkHeaderView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init();
  }

  public BookmarkHeaderView(@NonNull Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    init();
  }

  private void init() {
    setOrientation(LinearLayout.VERTICAL);
    View.inflate(getContext(), R.layout.item_guide_info, this);
    mImageView = findViewById(R.id.guide_image);
    mTitle = findViewById(R.id.guide_title);
    mDescriptionBtn = findViewById(R.id.btn_description);
    mAuthorTextView = findViewById(R.id.content_by);
    mImageViewLogo = findViewById(R.id.logo);
  }

  public void setCategory(@NonNull BookmarkCategory category)
  {
    if (!category.isMyCategory())
    {
      final Context context = getContext();
      final String imageUrl = category.getImageUrl();
      if (TextUtils.isEmpty(imageUrl) || !ConnectionState.INSTANCE.isConnected())
      {
        UiUtils.hide(mImageView);
      }
      else
      {
        Glide.with(context)
             .load(imageUrl)
             .placeholder(R.drawable.ic_placeholder)
             .centerCrop()
             .into(mImageView);
      }
      mTitle.setText(category.getName());
      boolean isHideDescriptionBtn = TextUtils.isEmpty(category.getDescription())
                                     && TextUtils.isEmpty(category.getAnnotation());
      UiUtils.hideIf(isHideDescriptionBtn, mDescriptionBtn);
      BookmarkCategory.Author author = category.getAuthor();
      if (author != null)
      {
        if (author.getId().equals(AUTHOR_LONELY_PLANET_ID))
          mImageViewLogo.setImageDrawable(context.getDrawable(R.drawable.ic_lp_logo));
        else
          UiUtils.hide(mImageViewLogo);

        CharSequence authorName = BookmarkCategory.Author.getContentByString(context, author);
        mAuthorTextView.setText(authorName);
      }
    }
    else
    {
      UiUtils.hide(this);
    }
  }
}
