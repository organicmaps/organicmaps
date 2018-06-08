package com.mapswithme.maps.bookmarks;

import android.graphics.drawable.Drawable;
import android.location.Location;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.PluralsRes;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkInfo;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
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

    void setAction(@NonNull HeaderAction action, @NonNull AdapterResourceProvider resProvider,
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

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ SECTION_TRACKS, SECTION_BMKS })
    public @interface Section {}

    final long mCategoryId;
    @NonNull
    private final View mView;

    BaseBookmarkHolder(@NonNull View itemView, long categoryId)
    {
      super(itemView);
      mCategoryId = categoryId;
      mView = itemView;
    }

    abstract void bind(int position);

    static boolean isSectionEmpty(long categoryId, @Section int section)
    {
      switch (section)
      {
        case SECTION_TRACKS:
          return BookmarkManager.INSTANCE.getTracksCount(categoryId) == 0;
        case SECTION_BMKS:
          return BookmarkManager.INSTANCE.getBookmarksCount(categoryId) == 0;
        default:
          throw new IllegalArgumentException("There is no section with index " + section);
      }
    }

    static int getSectionForPosition(long categoryId, int position)
    {
      if (position == getTracksSectionPosition(categoryId))
        return SECTION_TRACKS;
      if (position == getBookmarksSectionPosition(categoryId))
        return SECTION_BMKS;

      throw new IllegalArgumentException("There is no section in position " + position);
    }

    static int getTracksSectionPosition(long categoryId)
    {
      if (isSectionEmpty(categoryId, SECTION_TRACKS))
        return -1;

      return 0;
    }

    static int getBookmarksSectionPosition(long categoryId)
    {
      if (isSectionEmpty(categoryId, SECTION_BMKS))
        return -1;

      return BookmarkManager.INSTANCE.getTracksCount(categoryId)
             + (isSectionEmpty(categoryId, SECTION_TRACKS) ? 0 : 1);
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

    BookmarkViewHolder(@NonNull View itemView, long categoryId)
    {
      super(itemView, categoryId);
      mIcon = itemView.findViewById(R.id.iv__bookmark_color);
      mName = itemView.findViewById(R.id.tv__bookmark_name);
      mDistance = itemView.findViewById(R.id.tv__bookmark_distance);
    }

    @Override
    void bind(int position)
    {
      int pos = calculateBookmarkPosition(mCategoryId, position);
      final long bookmarkId = BookmarkManager.INSTANCE.getBookmarkIdByPosition(mCategoryId, pos);
      BookmarkInfo bookmark = new BookmarkInfo(mCategoryId, bookmarkId);
      mName.setText(bookmark.getTitle());
      final Location loc = LocationHelper.INSTANCE.getSavedLocation();
      if (loc != null)
      {
        final DistanceAndAzimut daa = bookmark.getDistanceAndAzimuth(loc.getLatitude(),
                                                                     loc.getLongitude(), 0.0);
        mDistance.setText(daa.getDistance());
      }
      else
        mDistance.setText(null);
      mIcon.setImageResource(bookmark.getIcon().getSelectedResId());
    }

    static int calculateBookmarkPosition(long categoryId, int position)
    {
      // Since bookmarks are always below tracks and header we should take it into account
      // during the bookmark's position calculation.
      return position - 1 - (isSectionEmpty(categoryId, SECTION_TRACKS)
                                ? 0 : BookmarkManager.INSTANCE.getTracksCount(categoryId) + 1);
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

    TrackViewHolder(@NonNull View itemView, long categoryId)
    {
      super(itemView, categoryId);
      mIcon = itemView.findViewById(R.id.iv__bookmark_color);
      mName = itemView.findViewById(R.id.tv__bookmark_name);
      mDistance = itemView.findViewById(R.id.tv__bookmark_distance);
    }

    @Override
    void bind(int position)
    {
      final long trackId = BookmarkManager.INSTANCE.getTrackIdByPosition(mCategoryId,
                                                                         position - 1);
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

    SectionViewHolder(@NonNull TextView itemView, long categoryId)
    {
      super(itemView, categoryId);
      mView = itemView;
    }

    @Override
    void bind(int position)
    {
      final int sectionIndex = getSectionForPosition(mCategoryId, position);
      mView.setText(getSections().get(sectionIndex));
      mView.setText(getSections().get(sectionIndex));
    }

    private List<String> getSections()
    {
      final List<String> sections = new ArrayList<>();
      sections.add(mView.getContext().getString(R.string.tracks));
      sections.add(mView.getContext().getString(R.string.bookmarks));
      return sections;
    }
  }
}
