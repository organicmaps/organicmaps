package app.organicmaps.bookmarks;

import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.View;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.CallSuper;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.PluralsRes;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.adapter.OnItemClickListener;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.IconClickListener;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.utils.Graphics;
import app.organicmaps.widget.recycler.DividerBehavior;
import app.organicmaps.widget.recycler.RecyclerClickListener;
import app.organicmaps.widget.recycler.RecyclerLongClickListener;

public class Holders
{
  /**
   * In selection mode the row itself is the only touch target, which is also why the colour circle never opens
   * its picker there. Making a child non-clickable is not enough: a pressed row dispatches the state down, so a
   * child keeping a borderless ripple would still splash. A child also has to leave the accessibility tree: its
   * content description names the action it no longer performs, and a non-clickable child is folded into the
   * row, so a screen reader would read it out as a part of it.
   */
  private static void bindSelectableChild(@NonNull View child, @Nullable Drawable background, boolean selectionMode)
  {
    child.setClickable(!selectionMode);
    child.setBackground(selectionMode ? null : background);
    child.setImportantForAccessibility(selectionMode ? View.IMPORTANT_FOR_ACCESSIBILITY_NO
                                                     : View.IMPORTANT_FOR_ACCESSIBILITY_YES);
  }

  /**
   * A holder of a row that is drawn as a part of a rounded section card.
   */
  static abstract class CardViewHolderBase extends RecyclerView.ViewHolder implements DividerBehavior
  {
    private boolean mSkipDivider;
    @DrawableRes
    private int mCardBackgroundRes;

    CardViewHolderBase(@NonNull View itemView)
    {
      super(itemView);
    }

    /**
     * Rounds the corners the row shares with the edges of its section card and suppresses the divider below the
     * last row, so that a section reads as a single card.
     */
    void bindCardPosition(boolean first, boolean last)
    {
      mSkipDivider = last;
      final int background = getCardBackground(first, last);
      // Rebinding the same background restarts an in-flight ripple.
      if (mCardBackgroundRes == background)
        return;
      mCardBackgroundRes = background;
      itemView.setBackgroundResource(background);
    }

    @Override
    public boolean skipDivider()
    {
      return mSkipDivider;
    }

    @DrawableRes
    private static int getCardBackground(boolean first, boolean last)
    {
      if (first)
        return last ? R.drawable.bg_card_item_single : R.drawable.bg_card_item_top;
      return last ? R.drawable.bg_card_item_bottom : R.drawable.bg_card_item_middle;
    }
  }

  public static class GeneralViewHolder extends RecyclerView.ViewHolder implements DividerBehavior
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

    @Override
    public boolean useFullWidthDivider()
    {
      return true;
    }
  }

  public static class HeaderViewHolder extends RecyclerView.ViewHolder implements DividerBehavior
  {
    @NonNull
    private final TextView mButton;
    @NonNull
    private final TextView mText;
    private boolean mSkipDivider;

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

    void setAction(@NonNull HeaderAction action, final boolean showAll)
    {
      mButton.setText(showAll ? R.string.bookmark_lists_show_all : R.string.bookmark_lists_hide_all);
      mButton.setOnClickListener(new ToggleShowAllClickListener(action, showAll));
    }

    void setAction(@NonNull HeaderActionChildCategories action, final boolean showAll)
    {
      mButton.setText(showAll ? R.string.bookmark_lists_show_all : R.string.bookmark_lists_hide_all);
      mButton.setOnClickListener(new ToggleShowAllChildCategoryClickListener(action, showAll));
    }

    /**
     * Opt-in for the card-grouped bookmark list, where a header is a label above the card and must not be underlined.
     */
    void setSkipDivider(boolean skip)
    {
      mSkipDivider = skip;
    }

    @Override
    public boolean skipDivider()
    {
      return mSkipDivider;
    }

    @Override
    public boolean useFullWidthDivider()
    {
      return true;
    }

    public interface HeaderAction
    {
      void onHideAll();

      void onShowAll();
    }

    public interface HeaderActionChildCategories
    {
      void onHideAll();

      void onShowAll();
    }

    private static class ToggleShowAllChildCategoryClickListener implements View.OnClickListener
    {
      private final HeaderActionChildCategories mAction;
      private final boolean mShowAll;

      ToggleShowAllChildCategoryClickListener(@NonNull HeaderActionChildCategories action, boolean showAll)
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

  static class CategoryViewHolderBase extends CardViewHolderBase
  {
    @Nullable
    protected BookmarkCategory mEntity;

    @NonNull
    protected final TextView mSize;

    public CategoryViewHolderBase(@NonNull View root)
    {
      super(root);
      mSize = root.findViewById(R.id.size);
    }

    @Override
    public boolean useFullWidthDivider()
    {
      return false;
    }

    protected void setSize()
    {
      if (mEntity == null)
        return;

      mSize.setText(getSizeString());
    }

    private String getSizeString()
    {
      final Resources resources = mSize.getResources();
      final int bookmarksCount = mEntity.getBookmarksCount();
      final int tracksCount = mEntity.getTracksCount();

      if ((bookmarksCount == 0 && tracksCount == 0) || (bookmarksCount > 0 && tracksCount > 0))
      {
        final String bookmarks = getQuantified(resources, R.plurals.bookmarks_places, bookmarksCount);
        final String tracks = getQuantified(resources, R.plurals.tracks, tracksCount);
        final String template = resources.getString(R.string.comma_separated_pair);

        return String.format(template, bookmarks, tracks);
      }

      if (bookmarksCount > 0)
        return getQuantified(resources, R.plurals.bookmarks_places, bookmarksCount);

      return getQuantified(resources, R.plurals.tracks, tracksCount);
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

    private String getQuantified(Resources resources, @PluralsRes int plural, int size)
    {
      return resources.getQuantityString(plural, size, size);
    }
  }
  static class CollectionViewHolder extends CategoryViewHolderBase
  {
    @NonNull
    private final View mView;
    @NonNull
    private final TextView mName;
    @NonNull
    private final CheckBox mVisibilityMarker;

    CollectionViewHolder(@NonNull View root)
    {
      super(root);
      mView = root;
      mName = root.findViewById(R.id.name);
      mVisibilityMarker = root.findViewById(R.id.checkbox);
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
  }

  static class CategoryViewHolder extends CategoryViewHolderBase
  {
    @NonNull
    private final TextView mName;
    @NonNull
    CheckBox mVisibilityMarker;
    @NonNull
    ImageView mMoreButton;

    CategoryViewHolder(@NonNull View root)
    {
      super(root);
      mName = root.findViewById(R.id.name);
      mVisibilityMarker = root.findViewById(R.id.checkbox);
      mMoreButton = root.findViewById(R.id.more);
    }

    void setVisibilityState(boolean visible)
    {
      mVisibilityMarker.setChecked(visible);
    }

    void setVisibilityListener(@Nullable View.OnClickListener listener)
    {
      mVisibilityMarker.setOnClickListener(listener);
    }

    void setMoreButtonClickListener(@Nullable View.OnClickListener listener)
    {
      mMoreButton.setOnClickListener(listener);
    }

    void setName(@NonNull String name)
    {
      mName.setText(name);
    }
  }

  static abstract class BaseBookmarkHolder extends CardViewHolderBase
  {
    @NonNull
    private final View mView;
    private boolean mSelectionMode;
    private boolean mSelected;
    /**
     * The checkbox is not focusable, so the row itself reports the checked state to a screen reader.
     */
    @NonNull
    private final AccessibilityDelegateCompat mSelectionDelegate = new AccessibilityDelegateCompat() {
      @Override
      public void onInitializeAccessibilityNodeInfo(@NonNull View host, @NonNull AccessibilityNodeInfoCompat info)
      {
        super.onInitializeAccessibilityNodeInfo(host, info);
        info.setCheckable(mSelectionMode);
        info.setChecked(mSelected ? AccessibilityNodeInfo.CHECKED_STATE_TRUE
                                  : AccessibilityNodeInfo.CHECKED_STATE_FALSE);
      }
    };

    BaseBookmarkHolder(@NonNull View itemView)
    {
      super(itemView);
      mView = itemView;
    }

    abstract void bind(@NonNull SectionPosition position,
                       @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource);

    /**
     * Applies the multi-selection state to the row. Rows that cannot be selected (section titles, the category
     * description) only record it.
     */
    @CallSuper
    void bindSelection(boolean selectionMode, boolean selected)
    {
      mSelectionMode = selectionMode;
      mSelected = selected;
    }

    /**
     * Must run on every bind, not once in the constructor: RecyclerView restores the delegate it saved on bind
     * when a holder goes to the pool, and it saves nothing while a screen reader is off.
     */
    void bindSelectionAccessibility()
    {
      ViewCompat.setAccessibilityDelegate(mView, mSelectionDelegate);
    }

    void setOnClickListener(@Nullable RecyclerClickListener listener)
    {
      mView.setOnClickListener(v -> {
        if (listener != null)
          listener.onItemClick(v, getBindingAdapterPosition());
      });
    }

    void setOnLongClickListener(@Nullable RecyclerLongClickListener listener)
    {
      mView.setOnLongClickListener(v -> {
        if (listener != null)
          listener.onLongItemClick(v, getBindingAdapterPosition());
        return true;
      });
    }

    @Override
    public boolean useFullWidthDivider()
    {
      return false;
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
    private final CheckBox mSelectionMarker;
    @NonNull
    private final ImageView mMoreButton;
    @Nullable
    private final Drawable mIconBackground;

    BookmarkViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mIcon = itemView.findViewById(R.id.iv__bookmark_color);
      mName = itemView.findViewById(R.id.tv__bookmark_name);
      mDistance = itemView.findViewById(R.id.tv__bookmark_distance);
      mSelectionMarker = itemView.findViewById(R.id.selection_checkbox);
      mMoreButton = itemView.findViewById(R.id.more);
      mIconBackground = mIcon.getBackground();
    }

    @Override
    void bindSelection(boolean selectionMode, boolean selected)
    {
      super.bindSelection(selectionMode, selected);
      bindSelectionAccessibility();
      UiUtils.showIf(selectionMode, mSelectionMarker);
      mSelectionMarker.setChecked(selected);
      // In selection mode every tap on the row must toggle it, so the inner targets step aside.
      UiUtils.hideIf(selectionMode, mMoreButton);
      bindSelectableChild(mIcon, mIconBackground, selectionMode);
    }

    public void setMoreButtonClickListener(@Nullable RecyclerClickListener listener)
    {
      mMoreButton.setOnClickListener(v -> {
        if (listener != null)
          listener.onItemClick(v, getBindingAdapterPosition());
      });
    }

    @Override
    void bind(@NonNull SectionPosition position, @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      final long bookmarkId = sectionsDataSource.getBookmarkId(position);
      BookmarkInfo bookmark = BookmarkManager.INSTANCE.getBookmarkInfo(bookmarkId);
      mName.setText(bookmark.getName());
      final Location loc = MwmApplication.from(mIcon.getContext()).getLocationHelper().getSavedLocation();

      String distanceValue =
          loc == null
              ? ""
              : bookmark.getDistance(loc.getLatitude(), loc.getLongitude(), 0.0).toString(mDistance.getContext());
      String separator = "";
      if (!distanceValue.isEmpty() && !bookmark.getFeatureType().isEmpty())
        separator = " • ";
      String subtitleValue = distanceValue.concat(separator).concat(bookmark.getFeatureType());
      mDistance.setText(subtitleValue);
      UiUtils.hideIf(TextUtils.isEmpty(subtitleValue), mDistance);

      mIcon.setImageResource(bookmark.getIcon().getResId());
      Drawable circle =
          Graphics.drawCircleAndImage(bookmark.getIcon().argb(), R.dimen.track_circle_size,
                                      bookmark.getIcon().getResId(), R.dimen.bookmark_icon_size, mIcon.getContext());
      mIcon.setImageDrawable(circle);
    }

    public void setBookmarkIconClickListener(IconClickListener listener)
    {
      mIcon.setOnClickListener(v -> listener.onItemClick(getBindingAdapterPosition()));
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
    private final ImageView mMoreButton;
    @NonNull
    private final ImageView mEyeIcon;
    @NonNull
    private final CheckBox mSelectionMarker;
    @Nullable
    private final Drawable mIconBackground;

    TrackViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mIcon = itemView.findViewById(R.id.iv__bookmark_color);
      mName = itemView.findViewById(R.id.tv__bookmark_name);
      mDistance = itemView.findViewById(R.id.tv__bookmark_distance);
      mMoreButton = itemView.findViewById(R.id.more);
      mEyeIcon = itemView.findViewById(R.id.eye);
      mSelectionMarker = itemView.findViewById(R.id.selection_checkbox);
      mIconBackground = mIcon.getBackground();
    }

    @Override
    void bindSelection(boolean selectionMode, boolean selected)
    {
      super.bindSelection(selectionMode, selected);
      bindSelectionAccessibility();
      UiUtils.showIf(selectionMode, mSelectionMarker);
      mSelectionMarker.setChecked(selected);
      // In selection mode every tap on the row must toggle it, so the inner targets step aside.
      UiUtils.hideIf(selectionMode, mEyeIcon, mMoreButton);
      bindSelectableChild(mIcon, mIconBackground, selectionMode);
    }

    @Override
    void bind(@NonNull SectionPosition position, @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      final long trackId = sectionsDataSource.getTrackId(position);
      Track track = BookmarkManager.INSTANCE.getTrack(trackId);
      mName.setText(track.getName());
      mDistance.setText(new StringBuilder()
                            .append(mDistance.getContext().getString(R.string.length))
                            .append(" ")
                            .append(track.getLength().toString(mDistance.getContext()))
                            .toString());
      Drawable circle =
          Graphics.drawCircle(track.getColor(), R.dimen.track_circle_size, mIcon.getContext().getResources());
      mIcon.setImageDrawable(circle);
      updateEyeIcon(track);
    }

    private void updateEyeIcon(@NonNull Track track)
    {
      mEyeIcon.setImageResource(track.isVisible() ? R.drawable.ic_show : R.drawable.ic_hide);
      mEyeIcon.setContentDescription(
          mEyeIcon.getContext().getString(track.isVisible() ? R.string.hide_track : R.string.show_track));
    }

    public void setMoreButtonClickListener(@Nullable RecyclerClickListener listener)
    {
      mMoreButton.setOnClickListener(v -> {
        if (listener != null)
          listener.onItemClick(v, getBindingAdapterPosition());
      });
    }

    public void setEyeClickListener(RecyclerClickListener listener)
    {
      mEyeIcon.setOnClickListener(v -> listener.onItemClick(v, getBindingAdapterPosition()));
    }

    public void setTrackIconClickListener(IconClickListener listener)
    {
      mIcon.setOnClickListener(v -> listener.onItemClick(getBindingAdapterPosition()));
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
    void bind(@NonNull SectionPosition position, @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      mView.setText(sectionsDataSource.getTitle(position.getSectionIndex(), mView.getResources()));
    }

    /** A section title is a label above the card, not a row of it. */
    @Override
    void bindCardPosition(boolean first, boolean last)
    {}

    @Override
    public boolean skipDivider()
    {
      return true;
    }
  }

  static class DescriptionViewHolder extends BaseBookmarkHolder
  {
    static final float SPACING_MULTIPLE = 1.0f;
    static final float SPACING_ADD = 0.0f;
    @NonNull
    private final TextView mTitle;
    @NonNull
    private final TextView mDescText;

    DescriptionViewHolder(@NonNull View itemView, @NonNull BookmarkCategory category)
    {
      super(itemView);
      mDescText = itemView.findViewById(R.id.text);
      mTitle = itemView.findViewById(R.id.title);
    }

    @Override
    void bind(@NonNull SectionPosition position, @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      mTitle.setText(sectionsDataSource.getCategory().getName());
      bindDescription(sectionsDataSource.getCategory());
    }

    private void bindDescription(@NonNull BookmarkCategory category)
    {
      String desc = TextUtils.isEmpty(category.getAnnotation()) ? category.getDescription() : category.getAnnotation();

      String formattedDesc = desc.replace("\n", "<br>");
      Spanned spannedDesc = Utils.fromHtml(formattedDesc);
      mDescText.setText(spannedDesc);

      UiUtils.showIf(!TextUtils.isEmpty(spannedDesc), mDescText);
    }
  }
}
