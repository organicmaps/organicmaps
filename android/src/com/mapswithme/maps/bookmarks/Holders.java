package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.text.Html;
import android.text.Layout;
import android.text.Spanned;
import android.text.StaticLayout;
import android.text.TextUtils;
import android.view.View;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.PluralsRes;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkInfo;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

public class Holders
{
  public static class GeneralViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mText;
    @NonNull
    private final ImageView mImage;

    GeneralViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mImage = itemView.findViewById(R.id.image);
      mText = itemView.findViewById(R.id.text);
    }

    @NonNull
    public TextView getText()
    {
      return mText;
    }

    @NonNull
    public ImageView getImage()
    {
      return mImage;
    }
  }

  public static class HeaderViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private TextView mButton;
    @NonNull
    private TextView mText;


    HeaderViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mButton = itemView.findViewById(R.id.button);
      mText = itemView.findViewById(R.id.text_message);
    }

    @NonNull
    public TextView getText()
    {
      return mText;
    }

    @NonNull
    public TextView getButton()
    {
      return mButton;
    }

    void setAction(@NonNull HeaderAction action,
                   @NonNull BookmarkCategoriesPageResProvider resProvider,
                   final boolean showAll)
    {
      mButton.setText(showAll
                      ? resProvider.getHeaderBtn().getSelectModeText()
                      : resProvider.getHeaderBtn().getUnSelectModeText());
      mButton.setOnClickListener(new ToggleShowAllClickListener(action, showAll));
    }

    void setAction(@NonNull HeaderActionChildCategories action,
                   final boolean showAll,
                   @BookmarkManager.CompilationType int compilationType,
                   @NonNull String serverId)
    {
      mButton.setText(showAll
                      ? R.string.bookmarks_groups_show_all
                      : R.string.bookmarks_groups_hide_all);
      mButton.setOnClickListener(new ToggleShowAllChildCategoryClickListener(
          action, showAll, compilationType, serverId));
    }

    public interface HeaderAction
    {
      void onHideAll();

      void onShowAll();
    }

    public interface HeaderActionChildCategories
    {
      void onHideAll(@BookmarkManager.CompilationType int compilationType);

      void onShowAll(@BookmarkManager.CompilationType int compilationType);
    }

    private static class ToggleShowAllChildCategoryClickListener implements View.OnClickListener
    {
      private final HeaderActionChildCategories mAction;
      private final boolean mShowAll;
      @BookmarkManager.CompilationType
      private final int mCompilationType;
      @NonNull
      private final String mServerId;
      @NonNull
      private final String mCompilationTypeString;

      ToggleShowAllChildCategoryClickListener(@NonNull HeaderActionChildCategories action,
                                              boolean showAll,
                                              @BookmarkManager.CompilationType int compilationType,
                                              @NonNull String serverId)
      {
        mAction = action;
        mShowAll = showAll;
        mCompilationType = compilationType;
        mServerId = serverId;
        mCompilationTypeString = compilationType == BookmarkManager.CATEGORY ?
                                 Statistics.ParamValue.CATEGORY : Statistics.ParamValue.COLLECTION;
      }

      @Override
      public void onClick(View view)
      {
        if (mShowAll)
        {
          mAction.onShowAll(mCompilationType);
          Statistics.INSTANCE.trackGuideVisibilityChange(Statistics.ParamValue.SHOW_ALL, mServerId,
                                                         mCompilationTypeString);
        }
        else
        {
          mAction.onHideAll(mCompilationType);
          Statistics.INSTANCE.trackGuideVisibilityChange(Statistics.ParamValue.HIDE_ALL, mServerId,
                                                         mCompilationTypeString);
        }
      }
    }

    private static class ToggleShowAllClickListener implements View.OnClickListener
    {
      private final HeaderAction mAction;
      private final boolean mShowAll;

      ToggleShowAllClickListener(@NonNull HeaderAction action, boolean showAll)
      {
        mAction = action;
        mShowAll = showAll;
      }

      @Override
      public void onClick(View view)
      {
        if (mShowAll)
          mAction.onShowAll();
        else
          mAction.onHideAll();
      }
    }
  }

  static class CollectionViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final View mView;
    @NonNull
    private final TextView mName;
    @NonNull
    private final CheckBox mVisibilityMarker;
    @NonNull
    private final TextView mSize;
    @Nullable
    private BookmarkCategory mEntity;

    CollectionViewHolder(@NonNull View root)
    {
      super(root);
      mView = root;
      mName = root.findViewById(R.id.name);
      mVisibilityMarker = root.findViewById(R.id.checkbox);
      mSize = root.findViewById(R.id.size);
    }

    void setOnClickListener(@Nullable OnItemClickListener<BookmarkCategory> listener)
    {
      mView.setOnClickListener(v -> {
        if (listener != null && mEntity != null)
          listener.onItemClick(v, mEntity);
      });
    }

    void setVisibilityState(boolean visible)
    {
      mVisibilityMarker.setChecked(visible);
    }

    void setVisibilityListener(@Nullable View.OnClickListener listener)
    {
      mVisibilityMarker.setOnClickListener(listener);
    }

    void setName(@NonNull String name)
    {
      mName.setText(name);
    }

    void setSize(@PluralsRes int phrase, int size)
    {
      mSize.setText(mSize.getResources().getQuantityString(phrase, size, size));
    }

    void setEntity(@NonNull BookmarkCategory entity)
    {
      mEntity = entity;
    }

    @NonNull
    public BookmarkCategory getEntity()
    {
      if (mEntity == null)
        throw new AssertionError("BookmarkCategory is null");
      return mEntity;
    }
  }

  static class CategoryViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mName;
    @NonNull
    CheckBox mVisibilityMarker;
    @NonNull
    TextView mSize;
    @NonNull
    View mMore;
    @NonNull
    TextView mAuthorName;
    @NonNull
    TextView mAccessRule;
    @NonNull
    ImageView mAccessRuleImage;
    @Nullable
    private BookmarkCategory mEntity;

    CategoryViewHolder(@NonNull View root)
    {
      super(root);
      mName = root.findViewById(R.id.name);
      mVisibilityMarker = root.findViewById(R.id.checkbox);
      int left = root.getResources().getDimensionPixelOffset(R.dimen.margin_half_plus);
      int right = root.getResources().getDimensionPixelOffset(R.dimen.margin_base_plus);
      UiUtils.expandTouchAreaForView(mVisibilityMarker, 0, left, 0, right);
      mSize = root.findViewById(R.id.size);
      mMore = root.findViewById(R.id.more);
      mAuthorName = root.findViewById(R.id.author_name);
      mAccessRule = root.findViewById(R.id.access_rule);
      mAccessRuleImage = root.findViewById(R.id.access_rule_img);
    }

    void setVisibilityState(boolean visible)
    {
      mVisibilityMarker.setChecked(visible);
    }

    void setVisibilityListener(@Nullable View.OnClickListener listener)
    {
      mVisibilityMarker.setOnClickListener(listener);
    }

    void setMoreListener(@Nullable View.OnClickListener listener)
    {
      mMore.setOnClickListener(listener);
    }

    void setName(@NonNull String name)
    {
      mName.setText(name);
    }

    void setSize(@PluralsRes int phrase, int size)
    {
      mSize.setText(mSize.getResources().getQuantityString(phrase, size, size));
    }

    void setCategory(@NonNull BookmarkCategory entity)
    {
      mEntity = entity;
    }

    @NonNull
    public BookmarkCategory getEntity()
    {
      if (mEntity == null)
        throw new AssertionError("BookmarkCategory is null");
      return mEntity;
    }

    @NonNull
    public TextView getAuthorName()
    {
      return mAuthorName;
    }
  }

  static class BookmarkDescriptionHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final BookmarkHeaderView mDescriptionView;

    public BookmarkDescriptionHolder(@NonNull BookmarkHeaderView itemView)
    {
      super(itemView);
      mDescriptionView = itemView;
    }

    void bind(@NonNull BookmarkCategory category)
    {
      mDescriptionView.setCategory(category);
    }
  }

  static abstract class BaseBookmarkHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final View mView;

    BaseBookmarkHolder(@NonNull View itemView)
    {
      super(itemView);
      mView = itemView;
    }

    abstract void bind(@NonNull SectionPosition position,
                       @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource);

    void setOnClickListener(@Nullable RecyclerClickListener listener)
    {
      mView.setOnClickListener(v -> {
        if (listener != null)
          listener.onItemClick(v, getAdapterPosition());
      });
    }

    void setOnLongClickListener(@Nullable RecyclerLongClickListener listener)
    {
      mView.setOnLongClickListener(v -> {
        if (listener != null)
          listener.onLongItemClick(v, getAdapterPosition());
        return true;
      });
    }
  }

  static class BookmarkViewHolder extends BaseBookmarkHolder
  {
    @NonNull
    private final ImageView mIcon;
    @NonNull
    private final TextView mName;
    @NonNull
    private final TextView mDistance;
    @NonNull
    private View mMore;

    BookmarkViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mIcon = itemView.findViewById(R.id.iv__bookmark_color);
      mName = itemView.findViewById(R.id.tv__bookmark_name);
      mDistance = itemView.findViewById(R.id.tv__bookmark_distance);
      mMore = itemView.findViewById(R.id.more);
    }

    @Override
    void bind(@NonNull SectionPosition position,
              @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      final long bookmarkId = sectionsDataSource.getBookmarkId(position);
      BookmarkInfo bookmark = new BookmarkInfo(sectionsDataSource.getCategory().getId(),
                                               bookmarkId);
      mName.setText(bookmark.getName());
      final Location loc = LocationHelper.INSTANCE.getSavedLocation();

      String distanceValue = loc == null ? "" : bookmark.getDistance(loc.getLatitude(),
                                                                     loc.getLongitude(), 0.0);
      String separator = "";
      if (!distanceValue.isEmpty() && !bookmark.getFeatureType().isEmpty())
        separator = " • ";
      String subtitleValue = distanceValue.concat(separator).concat(bookmark.getFeatureType());
      mDistance.setText(subtitleValue);
      UiUtils.hideIf(TextUtils.isEmpty(subtitleValue), mDistance);

      mIcon.setImageResource(bookmark.getIcon().getResId());
      Drawable circle = Graphics.drawCircleAndImage(bookmark.getIcon().argb(),
                                                    R.dimen.track_circle_size,
                                                    bookmark.getIcon().getResId(),
                                                    R.dimen.bookmark_icon_size,
                                                    mIcon.getContext().getResources());
      mIcon.setImageDrawable(circle);

      UiUtils.visibleIf(sectionsDataSource.getCategory().isSharingOptionsAllowed(), mMore);
    }

    void setMoreListener(@Nullable RecyclerClickListener listener)
    {
      mMore.setOnClickListener(v -> {
        if (listener != null)
          listener.onItemClick(v, getAdapterPosition());
      });
    }
  }

  static class TrackViewHolder extends BaseBookmarkHolder
  {
    @NonNull
    private final ImageView mIcon;
    @NonNull
    private final TextView mName;
    @NonNull
    private final TextView mDistance;

    TrackViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mIcon = itemView.findViewById(R.id.iv__bookmark_color);
      mName = itemView.findViewById(R.id.tv__bookmark_name);
      mDistance = itemView.findViewById(R.id.tv__bookmark_distance);
    }

    @Override
    void bind(@NonNull SectionPosition position,
              @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      final long trackId = sectionsDataSource.getTrackId(position);
      Track track = BookmarkManager.INSTANCE.getTrack(trackId);
      mName.setText(track.getName());
      mDistance.setText(new StringBuilder().append(mDistance.getContext()
                                                            .getString(R.string.length))
                                           .append(" ")
                                           .append(track.getLengthString())
                                           .toString());
      Drawable circle = Graphics.drawCircle(track.getColor(), R.dimen.track_circle_size,
                                            mIcon.getContext().getResources());
      mIcon.setImageDrawable(circle);
    }
  }

  public static class SectionViewHolder extends BaseBookmarkHolder
  {
    @NonNull
    private final TextView mView;

    SectionViewHolder(@NonNull TextView itemView)
    {
      super(itemView);
      mView = itemView;
    }

    @Override
    void bind(@NonNull SectionPosition position,
              @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      mView.setText(sectionsDataSource.getTitle(position.getSectionIndex(), mView.getResources()));
    }
  }

  static class DescriptionViewHolder extends BaseBookmarkHolder
  {
    static final float SPACING_MULTIPLE = 1.0f;
    static final float SPACING_ADD = 0.0f;
    @NonNull
    private final TextView mTitle;
    @NonNull
    private final TextView mAuthor;
    @NonNull
    private final TextView mDescText;
    @NonNull
    private final View mMoreBtn;

    DescriptionViewHolder(@NonNull View itemView, @NonNull BookmarkCategory category)
    {
      super(itemView);
      mDescText = itemView.findViewById(R.id.text);
      mTitle = itemView.findViewById(R.id.title);
      mAuthor = itemView.findViewById(R.id.author);

      mMoreBtn = itemView.findViewById(R.id.more_btn);
      boolean isEmptyDesc = TextUtils.isEmpty(category.getDescription());
      UiUtils.hideIf(isEmptyDesc, mMoreBtn);
      mMoreBtn.setOnClickListener(v -> onMoreBtnClicked(v, category));
    }

    private void onMoreBtnClicked(@NonNull View v, @NonNull BookmarkCategory category)
    {
      int lineCount = calcLineCount(mDescText, category.getDescription());
      mDescText.setMaxLines(lineCount);
      mDescText.setText(Html.fromHtml(category.getDescription()));
      v.setVisibility(View.GONE);
    }

    @Override
    void bind(@NonNull SectionPosition position,
              @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      mTitle.setText(sectionsDataSource.getCategory().getName());
      bindAuthor(sectionsDataSource.getCategory());
      bindDescriptionIfEmpty(sectionsDataSource.getCategory());
    }

    private void bindDescriptionIfEmpty(@NonNull BookmarkCategory category)
    {
      if (TextUtils.isEmpty(mDescText.getText()))
      {
        String desc = TextUtils.isEmpty(category.getAnnotation())
                      ? category.getDescription()
                      : category.getAnnotation();

        Spanned spannedDesc = Html.fromHtml(desc);
        mDescText.setText(spannedDesc);
      }
    }

    private void bindAuthor(@NonNull BookmarkCategory category)
    {
      BookmarkCategory.Author author = category.getAuthor();
      Context c = itemView.getContext();
      CharSequence authorName = author == null
                                ? null
                                : BookmarkCategory.Author.getRepresentation(c, author);
      mAuthor.setText(authorName);
    }

    private static int calcLineCount(@NonNull TextView textView, @NonNull String src)
    {
      StaticLayout staticLayout = new StaticLayout(src,
                                                   textView.getPaint(),
                                                   textView.getWidth(),
                                                   Layout.Alignment.ALIGN_NORMAL,
                                                   SPACING_MULTIPLE,
                                                   SPACING_ADD,
                                                   true);

      return staticLayout.getLineCount();
    }
  }
}
