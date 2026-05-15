package app.organicmaps.car.screens.maps.view;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import androidx.car.app.CarContext;
import androidx.car.app.ScreenManager;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.ListTemplate;
import androidx.car.app.model.Row;
import androidx.car.app.model.SectionedItemList;
import androidx.car.app.model.Template;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.car.R;
import app.organicmaps.car.screens.maps.DeleteMapsScreen;
import app.organicmaps.car.screens.maps.download.DownloadMapsScreen;
import app.organicmaps.car.screens.maps.download.DownloadMapsScreenBuilder;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;

final class MapsViewer
{
  private static CarIcon sArrowUp;
  private static CarIcon sArrowDown;
  private static CarIcon sDelete;
  private static CarIcon sUpdate;
  private static CarIcon sDownload;

  /**
   * Each navigation/pagination button appears in both the top and bottom control sections,
   * so it occupies this many rows of the total row budget.
   */
  private static final int ROWS_PER_CONTROL_BUTTON = 2;

  /**
   * Captures the full state of a folder level (current or historical) so it can be pushed onto
   * {@code mFolderHistory} and restored cheaply when the user presses back.
   *
   * <p>When {@code prebuiltRows} is {@code null} the state has been intentionally invalidated
   * (e.g. after a map download) and a fresh fetch will be triggered on restore.
   */
  private record FolderState(@NonNull String rootId, @NonNull String folderName, @Nullable List<Row> prebuiltRows,
                             int startIndex, @NonNull ArrayDeque<Integer> pageHistory,
                             @NonNull List<String> downloadableMapIds, @NonNull List<String> deletableMapIds)
  {
    FolderState
    {
      pageHistory = new ArrayDeque<>(pageHistory); // Defensive copy
    }

    /** Creates the initial loading state for the given folder. */
    @NonNull
    static FolderState initial(@NonNull String rootId, @NonNull String folderName)
    {
      return new FolderState(rootId, folderName, null, 0, new ArrayDeque<>(), new ArrayList<>(), new ArrayList<>());
    }

    /**
     * Returns a copy with rows and ID lists replaced and pagination set to
     * {@code newStartIndex} / {@code newPageHistory}. Used when a data load completes.
     */
    @NonNull
    FolderState withRowsResult(int newStartIndex, @NonNull ArrayDeque<Integer> newPageHistory,
                               @NonNull RowsBuildResult result)
    {
      return new FolderState(rootId, folderName, result.rows(), newStartIndex, newPageHistory,
                             result.downloadableMapIds(), result.deletableMapIds());
    }

    /**
     * Returns a copy with an updated start index for pagination.
     * <b>Mutate {@link #pageHistory()} before calling this</b> — the already-mutated deque is
     * snapshotted into the new state via the compact constructor's defensive copy.
     */
    @NonNull
    FolderState withStartIndex(int newStartIndex)
    {
      return new FolderState(rootId, folderName, prebuiltRows, newStartIndex, pageHistory, downloadableMapIds,
                             deletableMapIds);
    }
  }

  /** Immutable result of {@link #buildPrebuiltRows} — no side effects on {@code MapsViewer}. */
  private record RowsBuildResult(@NonNull List<Row> rows, @NonNull List<String> downloadableMapIds,
                                 @NonNull List<String> deletableMapIds)
  {}

  @NonNull
  private final CarContext mContext;
  @NonNull
  private final OrganicMaps mOrganicMapsContext;
  private final int mMaxRowsAllowed;
  @NonNull
  private final MapsProvider.Type mType;
  @NonNull
  private final Runnable mInvalidateCallback;
  @Nullable
  private Runnable mOnMapChangedCallback;

  private volatile boolean mIsNavigating;

  /** All mutable state for the currently displayed folder. */
  @NonNull
  private FolderState mState;
  /** Stack of parent folder states for recursive folder back-navigation. */
  @NonNull
  private final ArrayDeque<FolderState> mFolderHistory = new ArrayDeque<>();

  public MapsViewer(@NonNull CarContext context, @NonNull OrganicMaps organicMapsContext, int maxRowsAllowed,
                    @NonNull MapsProvider.Type type, @NonNull Runnable invalidateCallback)
  {
    initIcons(context);
    mContext = context;
    mOrganicMapsContext = organicMapsContext;
    mMaxRowsAllowed = maxRowsAllowed;
    mType = type;
    mInvalidateCallback = invalidateCallback;
    mState = FolderState.initial(CountryItem.getRootId(), mContext.getString(R.string.downloader_status_maps));
    mIsNavigating = RoutingController.get().isNavigating();
    final String initialRootId = mState.rootId();
    MapsProvider.getMaps(initialRootId, mType, maps -> onMapsLoaded(maps, mIsNavigating, initialRootId));
  }

  /**
   * Sets a callback that is invoked when a map download completes, so that sibling
   * {@link MapsViewer} instances (other tabs) can refresh their own data.
   */
  public void setOnMapChangedCallback(@NonNull Runnable callback)
  {
    mOnMapChangedCallback = callback;
  }

  @NonNull
  public Template buildTemplate()
  {
    mIsNavigating = RoutingController.get().isNavigating();
    final ListTemplate.Builder builder = new ListTemplate.Builder();

    final List<Row> prebuiltRows = mState.prebuiltRows();
    if (prebuiltRows == null)
      builder.setLoading(true);
    else
    {
      // Determine navigation button visibility and how many map rows we can show.
      // The control section is added TWICE (above and below the maps list), so each
      // navigation button occupies ROWS_PER_CONTROL_BUTTON rows of the total budget.
      final boolean hasBack = !mFolderHistory.isEmpty();
      final boolean hasPrev = mState.startIndex() > 0;
      final int remaining = prebuiltRows.size() - mState.startIndex();
      // Rows left after reserving space for the "back" and "prev" buttons.
      final int baseAvailable =
          mMaxRowsAllowed - (hasBack ? ROWS_PER_CONTROL_BUTTON : 0) - (hasPrev ? ROWS_PER_CONTROL_BUTTON : 0);
      // We need "next" only when not all remaining maps fit in baseAvailable rows.
      final boolean hasNext = remaining > baseAvailable;
      // Reserve rows for the "next" button when it is present.
      final int mapsToShow = hasNext ? Math.max(1, baseAvailable - ROWS_PER_CONTROL_BUTTON) : remaining;
      final int mapsEndIndex = mState.startIndex() + mapsToShow;

      final boolean hasControls = hasBack || hasPrev || hasNext;
      if (hasControls)
        builder.addSectionedList(buildControlActionsSection(hasBack, hasPrev, hasNext, mapsEndIndex));
      final SectionedItemList mapsListSection = buildMapsListSection(prebuiltRows, mapsEndIndex);
      if (!mapsListSection.getItemList().getItems().isEmpty())
        builder.addSectionedList(mapsListSection);
      else
      {
        final ItemList.Builder emptyListBuilder = new ItemList.Builder();
        if (mType == MapsProvider.Type.Updatable)
          emptyListBuilder.setNoItemsMessage(mContext.getString(R.string.up_to_date));
        builder.setSingleList(emptyListBuilder.build());
      }
      if (hasControls)
        builder.addSectionedList(buildControlActionsSection(hasBack, hasPrev, hasNext, mapsEndIndex));

      addFabActions(builder);
    }

    return builder.build();
  }

  public boolean onBackPressed()
  {
    // First: go back one page within the current folder.
    if (!mState.pageHistory().isEmpty())
    {
      mState = mState.withStartIndex(mState.pageHistory().pop());
      mInvalidateCallback.run();
      return true;
    }
    // Then: go back to the parent folder (restores cached state, no re-fetch).
    if (!mFolderHistory.isEmpty())
    {
      restoreFolderState(mFolderHistory.pop());
      return true;
    }
    return false;
  }

  // ── Folder / data navigation ──────────────────────────────────────────────

  void refreshCurrentFolder()
  {
    // Invalidate cached parent folder states so that navigating back always
    // shows up-to-date download status after any map change.
    invalidateParentFolderStates();

    // Save the current pagination state so the user stays on the same page after the refresh.
    final int savedStartIndex = mState.startIndex();
    final ArrayDeque<Integer> savedPageHistory = new ArrayDeque<>(mState.pageHistory());
    final boolean hasFolderHistory = !mFolderHistory.isEmpty();

    // Show the loading spinner immediately, then re-fetch in the background.
    mState = FolderState.initial(mState.rootId(), mState.folderName());
    mInvalidateCallback.run();
    MapsProvider.getMaps(
        mState.rootId(), mType,
        maps -> onMapsRefreshed(maps, savedStartIndex, savedPageHistory, hasFolderHistory, mIsNavigating));
  }

  /**
   * Discards the cached rows in every parent {@link FolderState} so that pressing "back"
   * into any ancestor folder triggers a fresh fetch rather than restoring stale data.
   */
  private void invalidateParentFolderStates()
  {
    if (mFolderHistory.isEmpty())
      return;
    final ArrayDeque<FolderState> refreshed = new ArrayDeque<>(mFolderHistory.size());
    for (final FolderState state : mFolderHistory)
      refreshed.addLast(FolderState.initial(state.rootId(), state.folderName()));
    mFolderHistory.clear();
    mFolderHistory.addAll(refreshed);
  }

  private void openFolder(@NonNull String folderId, @NonNull String folderName)
  {
    // Push the current state so it can be restored on back-press.
    // mState is replaced immediately below, so no intermediate mutations can corrupt the pushed copy.
    mFolderHistory.push(mState);

    mState = FolderState.initial(folderId, folderName);

    // Show the loading spinner immediately, then fetch in the background.
    mInvalidateCallback.run();
    MapsProvider.getMaps(folderId, mType, maps -> onMapsLoaded(maps, mIsNavigating, folderId));
  }

  private void restoreFolderState(@NonNull FolderState state)
  {
    mState = state;
    mInvalidateCallback.run();
    // If the state was invalidated (e.g. after a download), trigger a fresh fetch.
    if (mState.prebuiltRows() == null)
    {
      final String restoredRootId = mState.rootId();
      MapsProvider.getMaps(restoredRootId, mType, maps -> onMapsLoaded(maps, mIsNavigating, restoredRootId));
    }
  }

  // ── Template sections ─────────────────────────────────────────────────────

  @NonNull
  private SectionedItemList buildControlActionsSection(boolean hasBack, boolean hasPrev, boolean hasNext,
                                                       int mapsEndIndex)
  {
    final ItemList.Builder builder = new ItemList.Builder();

    if (hasBack)
    {
      builder.addItem(new Row.Builder()
                          .setTitle(mContext.getString(R.string.back))
                          .setImage(CarIcon.BACK)
                          .setOnClickListener(() -> restoreFolderState(mFolderHistory.pop()))
                          .build());
    }

    if (hasPrev)
    {
      builder.addItem(new Row.Builder()
                          .setTitle(mContext.getString(R.string.back))
                          .setImage(sArrowUp)
                          .setOnClickListener(() -> {
                            mState =
                                mState.withStartIndex(mState.pageHistory().isEmpty() ? 0 : mState.pageHistory().pop());
                            mInvalidateCallback.run();
                          })
                          .build());
    }

    if (hasNext)
    {
      builder.addItem(new Row.Builder()
                          .setTitle(mContext.getString(R.string.next_button))
                          .setImage(sArrowDown)
                          .setOnClickListener(() -> {
                            // Mutate before withStartIndex copies the deque
                            mState.pageHistory().push(mState.startIndex());
                            mState = mState.withStartIndex(mapsEndIndex);
                            mInvalidateCallback.run();
                          })
                          .build());
    }

    return SectionedItemList.create(builder.build(), mContext.getString(R.string.controls));
  }

  @NonNull
  private SectionedItemList buildMapsListSection(@NonNull List<Row> prebuiltRows, int mapsEndIndex)
  {
    final ItemList.Builder listBuilder = new ItemList.Builder();
    for (int i = mState.startIndex(); i < mapsEndIndex; i++)
      listBuilder.addItem(prebuiltRows.get(i));

    return SectionedItemList.create(listBuilder.build(), mState.folderName());
  }

  // ── Data loading ──────────────────────────────────────────────────────────

  /**
   * Builds {@link Row} objects from {@code maps} on the worker thread (pure data transformation).
   * All state mutations happen on the UI thread after this returns.
   */
  @WorkerThread
  @NonNull
  private RowsBuildResult buildPrebuiltRows(@NonNull List<MapsProvider.MapItem> maps, boolean isNavigating)
  {
    final List<String> downloadableMapIds = new ArrayList<>();
    final List<String> deletableMapIds = new ArrayList<>();
    final List<Row> rows = new ArrayList<>(maps.size());
    for (final MapsProvider.MapItem map : maps)
    {
      final Row.Builder rowBuilder = new Row.Builder();
      rowBuilder.setTitle(map.name());
      rowBuilder.setImage(map.icon(mContext));
      if (map.browsable())
      {
        rowBuilder.setBrowsable(true);
        rowBuilder.setOnClickListener(() -> openFolder(map.id(), map.name()));
        rowBuilder.addText(
            String.format("%s: %s", mContext.getString(R.string.downloader_status_maps),
                          mContext.getString(R.string.downloader_of, map.childCount(), map.totalChildCount())));
      }
      else
      {
        rowBuilder.addText(map.description());
        if (!isNavigating)
        {
          if (map.downloadable())
            rowBuilder.setOnClickListener(() -> onDownloadMapClicked(new String[] {map.id()}));

          if (map.removable() && mType != MapsProvider.Type.Updatable)
            addRemoveMapAction(rowBuilder, map.id());
        }
      }
      rows.add(rowBuilder.build());

      if (map.needsDownload())
        downloadableMapIds.add(map.id());
      if (map.isDownloaded())
        deletableMapIds.add(map.id());
    }
    return new RowsBuildResult(rows, downloadableMapIds, deletableMapIds);
  }

  /** Called when a folder is opened for the first time — resets pagination to the first page. */
  @WorkerThread
  private void onMapsLoaded(@NonNull List<MapsProvider.MapItem> maps, boolean isNavigating, @NonNull String expectedRootId)
  {
    final RowsBuildResult result = buildPrebuiltRows(maps, isNavigating);
    UiThread.runLater(() -> {
      if (!mState.rootId().equals(expectedRootId))
        return;
      mState = mState.withRowsResult(0, new ArrayDeque<>(), result);
      mInvalidateCallback.run();
    });
  }

  /**
   * Called after {@link #refreshCurrentFolder()} — rebuilds rows but restores the saved
   * pagination position so the user stays on the same page.
   *
   * @param hasFolderHistory whether there was a parent folder on the stack at the time the
   *                         refresh was initiated (evaluated on the UI thread to avoid racing).
   */
  @WorkerThread
  private void onMapsRefreshed(@NonNull List<MapsProvider.MapItem> maps, int savedStartIndex,
                               @NonNull ArrayDeque<Integer> savedPageHistory, boolean hasFolderHistory,
                               boolean isNavigating)
  {
    // If the folder is now empty and there is a parent to return to, drop this folder
    // automatically (e.g. the last map in a sub-folder was removed).
    if (maps.isEmpty() && hasFolderHistory)
    {
      UiThread.runLater(() -> restoreFolderState(mFolderHistory.pop()));
      return;
    }

    final RowsBuildResult result = buildPrebuiltRows(maps, isNavigating);
    UiThread.runLater(() -> {
      // Clamp the saved index: the new list may be shorter (e.g. after a deletion).
      final int newStartIndex = (savedStartIndex < result.rows().size()) ? savedStartIndex : 0;
      mState = mState.withRowsResult(newStartIndex, savedPageHistory, result);
      mInvalidateCallback.run();
    });
  }

  private void addRemoveMapAction(@NonNull Row.Builder rowBuilder, @NonNull String mapId)
  {
    rowBuilder.addAction(new Action.Builder()
                             .setIcon(sDelete)
                             .setBackgroundColor(CarColor.RED)
                             .setOnClickListener(() -> onDeleteMapClicked(mapId))
                             .build());
  }

  private void onDeleteMapClicked(@NonNull String mapId)
  {
    final ScreenManager screenManager = mContext.getCarService(ScreenManager.class);
    if (DeleteMapsScreen.MARKER.equals(screenManager.getTop().getMarker()))
      return;
    screenManager.pushForResult(new DeleteMapsScreen(mContext, mOrganicMapsContext, new String[] {mapId}),
                                this::onMapsDeleted);
  }

  private void onDownloadMapClicked(@NonNull String[] mapIds)
  {
    final ScreenManager screenManager = mContext.getCarService(ScreenManager.class);

    // Avoid double click
    if (DownloadMapsScreen.MARKER.equals(screenManager.getTop().getMarker()))
      return;

    final DownloadMapsScreenBuilder builder = new DownloadMapsScreenBuilder(mContext, mOrganicMapsContext);
    builder.setDownloaderType(DownloadMapsScreenBuilder.DownloaderType.Viewer);
    builder.setMissingMaps(mapIds);

    screenManager.pushForResult(builder.build(), this::onMapDownloaded);
  }

  private void onMapDownloaded(@Nullable Object result)
  {
    // Map was not downloaded
    if (!Boolean.TRUE.equals(result))
      return;

    if (mOnMapChangedCallback != null)
      mOnMapChangedCallback.run();
    else
      refreshCurrentFolder();
  }

  private void addFabActions(@NonNull ListTemplate.Builder builder)
  {
    if (mIsNavigating)
      return;

    // The Updatable tab shows an "Update All" FAB at every folder level, including root,
    // since the user may want to update all maps without navigating into sub-folders.
    if (mType == MapsProvider.Type.Updatable)
    {
      if (!mState.downloadableMapIds().isEmpty())
        builder.addAction(buildUpdateAllFab());
      return;
    }

    // For other tabs, FABs only appear inside sub-folders, not at root level.
    if (mFolderHistory.isEmpty())
      return;

    if (mType == MapsProvider.Type.All && !mState.downloadableMapIds().isEmpty())
      builder.addAction(buildDownloadAllFab());

    if (!mState.deletableMapIds().isEmpty())
      builder.addAction(buildDeleteAllFab());
  }

  @NonNull
  private Action buildUpdateAllFab()
  {
    return new Action.Builder()
        .setBackgroundColor(CarColor.BLUE)
        .setIcon(sUpdate)
        .setOnClickListener(() -> onDownloadMapClicked(mState.downloadableMapIds().toArray(new String[0])))
        .build();
  }

  @NonNull
  private Action buildDownloadAllFab()
  {
    return new Action.Builder()
        .setBackgroundColor(CarColor.BLUE)
        .setIcon(sDownload)
        .setOnClickListener(() -> onDownloadMapClicked(mState.downloadableMapIds().toArray(new String[0])))
        .build();
  }

  @NonNull
  private Action buildDeleteAllFab()
  {
    return new Action.Builder()
        .setBackgroundColor(CarColor.RED)
        .setIcon(sDelete)
        .setOnClickListener(this::onDeleteAllClicked)
        .build();
  }

  private void onDeleteAllClicked()
  {
    final ScreenManager screenManager = mContext.getCarService(ScreenManager.class);
    // Avoid double-tap opening a second DeleteMapsScreen.
    if (DeleteMapsScreen.MARKER.equals(screenManager.getTop().getMarker()))
      return;
    screenManager.pushForResult(
        new DeleteMapsScreen(mContext, mOrganicMapsContext, mState.deletableMapIds().toArray(new String[0])),
        this::onMapsDeleted);
  }

  private void onMapsDeleted(@Nullable Object result)
  {
    if (!Boolean.TRUE.equals(result))
      return;
    if (mOnMapChangedCallback != null)
      mOnMapChangedCallback.run();
    else
      refreshCurrentFolder();
  }

  private static synchronized void initIcons(@NonNull CarContext context)
  {
    if (sArrowUp == null)
      sArrowUp = new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_arrow_up)).build();
    if (sArrowDown == null)
      sArrowDown = new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_arrow_down)).build();
    if (sDelete == null)
      sDelete = new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_delete)).build();
    if (sUpdate == null)
      sUpdate = new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_downloader_update)).build();
    if (sDownload == null)
      sDownload = new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_download)).build();
  }
}
