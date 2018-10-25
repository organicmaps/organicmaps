package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.PluralsRes;
import android.support.v7.widget.RecyclerView;
import android.text.Layout;
import android.text.StaticLayout;
import android.text.TextUtils;
import android.view.View;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkInfo;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

public class Holders
{
  static class GeneralViewHolder extends RecyclerView.ViewHolder
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

  static class HeaderViewHolder extends RecyclerView.ViewHolder
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

    public interface HeaderAction
    {
      void onHideAll();
      void onShowAll();
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

  static abstract class BaseBookmarkHolder extends RecyclerView.ViewHolder
  {
    static final int SECTION_TRACKS = 0;
    static final int SECTION_BMKS = 1;
    static final int SECTION_DESC = 2;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ SECTION_TRACKS, SECTION_BMKS, SECTION_DESC})
    public @interface Section {}

    @NonNull
    private final View mView;

    BaseBookmarkHolder(@NonNull View itemView)
    {
      super(itemView);
      mView = itemView;
    }

    abstract void bind(int position, @NonNull BookmarkCategory category);

    static int calculateTrackPosition(@NonNull BookmarkCategory category, int position)
    {
      return position
             - (isSectionEmpty(category, SECTION_TRACKS) ? 0 : 1)
             - getDescItemCount(category);
    }

    static boolean isSectionEmpty(@NonNull BookmarkCategory category, @Section int section)
    {
      switch (section)
      {
        case SECTION_TRACKS:
          return category.getTracksCount() == 0;
        case SECTION_BMKS:
          return category.getBookmarksCount() == 0;
        case SECTION_DESC:
          return TextUtils.isEmpty(category.getDescription()) && TextUtils.isEmpty(category.getAnnotation());
        default:
          throw new IllegalArgumentException("There is no section with index " + section);
      }
    }

    static int getSectionForPosition(@NonNull BookmarkCategory category, int position)
    {
      if (position == getDescSectionPosition(category))
        return SECTION_DESC;
      if (position == getTracksSectionPosition(category))
        return SECTION_TRACKS;
      if (position == getBookmarksSectionPosition(category))
        return SECTION_BMKS;

      throw new IllegalArgumentException("There is no section in position " + position);
    }

    static int getDescSectionPosition(@NonNull BookmarkCategory category)
    {
      if (isSectionEmpty(category, SECTION_DESC))
        return RecyclerView.NO_POSITION;

      return 0;
    }

    static int getTracksSectionPosition(@NonNull BookmarkCategory category)
    {
      if (isSectionEmpty(category, SECTION_TRACKS))
        return RecyclerView.NO_POSITION;

      return getDescItemCount(category);
    }

    static int getBookmarksSectionPosition(@NonNull BookmarkCategory category)
    {
      if (isSectionEmpty(category, SECTION_BMKS))
        return RecyclerView.NO_POSITION;

      int beforeCurrentSectionItemsCount = getTracksSectionPosition(category);
      return (beforeCurrentSectionItemsCount == RecyclerView.NO_POSITION
              ? getDescItemCount(category)
              : beforeCurrentSectionItemsCount)
             + getTrackItemCount(category);
    }

    private static int getTrackItemCount(@NonNull BookmarkCategory category)
    {
      return category.getTracksCount() + (isSectionEmpty(category, SECTION_TRACKS) ? 0 : 1);
    }

    static int getDescItemCount(@NonNull BookmarkCategory category)
    {
      return isSectionEmpty(category, SECTION_DESC) ? 0 : /* section header */  1 + /* non empty desc */ 1;
    }

    void setOnClickListener(@Nullable RecyclerClickListener listener)
    {
      mView.setOnClickListener(v -> {
        if (listener != null)
          listener.onItemClick(v, getAdapterPosition());
      });
    }

    void setOnLongClickListener(@Nullable RecyclerLongClickListener listener)
    {
      mView.setOnLongClickListener(v -> onOpenActionMenu(v, listener));
    }

    boolean onOpenActionMenu(@NonNull View v, @Nullable RecyclerLongClickListener listener)
    {
      if (listener != null)
        listener.onLongItemClick(v, getAdapterPosition());
      return true;
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

    BookmarkViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mIcon = itemView.findViewById(R.id.iv__bookmark_color);
      mName = itemView.findViewById(R.id.tv__bookmark_name);
      mDistance = itemView.findViewById(R.id.tv__bookmark_distance);
    }

    @Override
    void setOnLongClickListener(@Nullable RecyclerLongClickListener listener)
    {
      super.setOnLongClickListener(listener);
    }

    @Override
    void bind(int position, @NonNull BookmarkCategory category)
    {
      int pos = calculateBookmarkPosition(category, position);
      final long bookmarkId = BookmarkManager.INSTANCE.getBookmarkIdByPosition(category.getId(), pos);
      BookmarkInfo bookmark = new BookmarkInfo(category.getId(), bookmarkId);
      mName.setText(bookmark.getTitle());
      final Location loc = LocationHelper.INSTANCE.getSavedLocation();

      String distanceValue = loc == null ? null : bookmark.getDistance(loc.getLatitude(),
                                                                       loc.getLongitude(), 0.0);
      mDistance.setText(distanceValue);
      UiUtils.hideIf(TextUtils.isEmpty(distanceValue), mDistance);
      mIcon.setImageResource(bookmark.getIcon().getSelectedResId());
    }

    static int calculateBookmarkPosition(@NonNull BookmarkCategory category, int position)
    {
      // Since bookmarks are always below tracks and header we should take it into account
      // during the bookmark's position calculation.
      return  calculateTrackPosition(category, position)
              - category.getTracksCount()
              - (isSectionEmpty(category, SECTION_BMKS) ? 0 : 1);
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
    void bind(int position, @NonNull BookmarkCategory category)
    {
      int relativePos = calculateTrackPosition(category, position);
      final long trackId = BookmarkManager.INSTANCE.getTrackIdByPosition(category.getId(), relativePos);
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

  static class SectionViewHolder extends BaseBookmarkHolder
  {
    @NonNull
    private final TextView mView;

    SectionViewHolder(@NonNull TextView itemView)
    {
      super(itemView);
      mView = itemView;
    }

    @Override
    void bind(int position, @NonNull BookmarkCategory category)
    {
      final int sectionIndex = getSectionForPosition(category, position);
      mView.setText(getSections().get(sectionIndex));
    }

    private List<String> getSections()
    {
      final List<String> sections = new ArrayList<>();
      sections.add(mView.getContext().getString(R.string.tracks_title));
      sections.add(mView.getContext().getString(R.string.bookmarks));
      sections.add(mView.getContext().getString(R.string.description));
      return sections;
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
      mDescText.setText(category.getDescription());
      v.setVisibility(View.GONE);
    }

    @Override
    void bind(int position, @NonNull BookmarkCategory category)
    {
      mTitle.setText(category.getName());
      bindAuthor(category);
      bindDescriptionIfEmpty(category);
    }

    private void bindDescriptionIfEmpty(@NonNull BookmarkCategory category)
    {
      if (TextUtils.isEmpty(mDescText.getText()))
      {
        String desc = TextUtils.isEmpty(category.getAnnotation())
                         ? category.getDescription()
                         : category.getAnnotation();
        mDescText.setText(desc);
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
