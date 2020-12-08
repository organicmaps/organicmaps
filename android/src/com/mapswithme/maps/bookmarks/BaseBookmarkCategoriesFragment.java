package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;

import androidx.annotation.CallSuper;
import androidx.annotation.IdRes;
import androidx.annotation.LayoutRes;
import androidx.annotation.MenuRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import com.cocosw.bottomsheet.BottomSheet;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.base.DataChangedListener;
import com.mapswithme.maps.bookmarks.data.AbstractCategoriesSnapshot;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.BookmarkSharingResult;
import com.mapswithme.maps.bookmarks.data.FilterStrategy;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.maps.ugc.routes.UgcRouteEditSettingsActivity;
import com.mapswithme.maps.ugc.routes.UgcRouteSharingOptionsActivity;
import com.mapswithme.maps.widget.PlaceholderView;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.statistics.Analytics;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;

public abstract class BaseBookmarkCategoriesFragment extends BaseMwmRecyclerFragment<BookmarkCategoriesAdapter>
    implements EditTextDialogFragment.EditTextDialogInterface,
               MenuItem.OnMenuItemClickListener,
               BookmarkManager.BookmarksLoadingListener,
               CategoryListCallback,
               KmlImportController.ImportKmlCallback,
               OnItemClickListener<BookmarkCategory>,
               OnItemLongClickListener<BookmarkCategory>

{
  static final int REQ_CODE_CATALOG = 101;
  static final int REQ_CODE_DELETE_CATEGORY = 102;

  private static final int MAX_CATEGORY_NAME_LENGTH = 60;

  @Nullable
  private BookmarkCategory mSelectedCategory;
  @Nullable
  private CategoryEditor mCategoryEditor;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkManager.BookmarksCatalogListener mCatalogListener;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private DataChangedListener mCategoriesAdapterObserver;

  @Override
  @LayoutRes
  protected int getLayoutRes()
  {
    return R.layout.fragment_bookmark_categories;
  }

  @NonNull
  @Override
  protected BookmarkCategoriesAdapter createAdapter()
  {
    FilterStrategy strategy = getType().getFilterStrategy();
    List<BookmarkCategory> items = BookmarkManager.INSTANCE.getCategoriesSnapshot(strategy)
                                                           .getItems();
    return new BookmarkCategoriesAdapter(requireContext(), getType(), items);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    onPrepareControllers(view);
    getAdapter().setOnClickListener(this);
    getAdapter().setOnLongClickListener(this);
    getAdapter().setCategoryListCallback(this);

    RecyclerView rw = getRecyclerView();
    if (rw == null) return;

    rw.setNestedScrollingEnabled(false);
    RecyclerView.ItemDecoration decor = ItemDecoratorFactory
        .createDecoratorWithPadding(getContext());
    rw.addItemDecoration(decor);
    mCatalogListener = new CatalogListenerDecorator(createCatalogListener(), this);
    mCategoriesAdapterObserver = new CategoriesAdapterObserver(this);
    BookmarkManager.INSTANCE.addCategoriesUpdatesListener(mCategoriesAdapterObserver);
  }

  protected void onPrepareControllers(@NonNull View view)
  {
    // No op
  }

  protected void updateLoadingPlaceholder()
  {
    View root = getView();
    if (root == null)
      throw new AssertionError("Fragment view must be non-null at this point!");

    View loadingPlaceholder = root.findViewById(R.id.placeholder_loading);
    boolean showLoadingPlaceholder = BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress();
    UiUtils.showIf(showLoadingPlaceholder, loadingPlaceholder);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addLoadingListener(this);
    BookmarkManager.INSTANCE.addCatalogListener(mCatalogListener);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeLoadingListener(this);
    BookmarkManager.INSTANCE.removeCatalogListener(mCatalogListener);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateLoadingPlaceholder();
    getAdapter().notifyDataSetChanged();
  }

  @Override
  public void onPause()
  {
    super.onPause();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(mCategoriesAdapterObserver);
  }

  @Override
  public boolean onMenuItemClick(MenuItem item)
  {
    MenuItemClickProcessorWrapper processor = MenuItemClickProcessorWrapper
        .getInstance(item.getItemId());

    processor
        .mInternalProcessor
        .process(this, getSelectedCategory());
    Statistics.INSTANCE.trackBookmarkListSettingsClick(processor.getAnalytics());
    return true;
  }

  protected final void showBottomMenu(@NonNull BookmarkCategory item)
  {
    mSelectedCategory = item;
    showBottomMenuInternal(item);
  }

  private void showBottomMenuInternal(@NonNull BookmarkCategory item)
  {
    BottomSheetHelper.Builder bs = BottomSheetHelper.create(getActivity(), item.getName())
                                                    .sheet(getCategoryMenuResId())
                                                    .listener(this);

    BottomSheet bottomSheet = bs.build();
    prepareBottomMenuItems(bottomSheet);
    MenuItem menuItem = BottomSheetHelper.findItemById(bottomSheet, R.id.show_on_map);
    menuItem.setIcon(item.isVisible() ? R.drawable.ic_hide : R.drawable.ic_show)
            .setTitle(item.isVisible() ? R.string.hide : R.string.show);
    BottomSheetHelper.tint(bottomSheet);
    bottomSheet.show();
  }

  protected abstract void prepareBottomMenuItems(@NonNull BottomSheet bottomSheet);

  @MenuRes
  protected int getCategoryMenuResId()
  {
    return R.menu.menu_bookmark_categories;
  }

  @Override
  public void onMoreOperationClick(@NonNull BookmarkCategory item)
  {
    showBottomMenu(item);
  }

  @Override
  protected void setupPlaceholder(@Nullable PlaceholderView placeholder)
  {
    // A placeholder is no needed on this screen.
  }

  @Override
  public void onBookmarksLoadingStarted()
  {
    updateLoadingPlaceholder();
  }

  @Override
  public void onBookmarksLoadingFinished()
  {
    updateLoadingPlaceholder();
    getAdapter().notifyDataSetChanged();
  }

  @Override
  public void onBookmarksFileLoaded(boolean success)
  {
    // Do nothing here.
  }

  @Override
  public void onFooterClick()
  {
    mCategoryEditor = BookmarkManager.INSTANCE::createCategory;

    EditTextDialogFragment.show(getString(R.string.bookmarks_create_new_group),
                                getString(R.string.bookmarks_new_list_hint),
                                getString(R.string.bookmark_set_name),
                                getString(R.string.create), getString(R.string.cancel),
                                MAX_CATEGORY_NAME_LENGTH, this);
  }

  @Override
  public void onFinishKmlImport()
  {
    getAdapter().notifyDataSetChanged();
  }

  @NonNull
  @Override
  public EditTextDialogFragment.OnTextSaveListener getSaveTextListener()
  {
    return this::onSaveText;
  }

  @NonNull
  @Override
  public EditTextDialogFragment.Validator getValidator()
  {
    return new CategoryValidator();
  }

  @NonNull
  protected abstract BookmarkCategory.Type getType();

  @Override
  public void onItemClick(@NonNull View v, @NonNull BookmarkCategory category)
  {
    mSelectedCategory = category;
    BookmarkListActivity.startForResult(requireActivity(), category);
  }

  protected void onShareActionSelected(@NonNull BookmarkCategory category)
  {
    SharingHelper.INSTANCE.prepareBookmarkCategoryForSharing(getActivity(), category.getId());
  }

  private void onDeleteActionSelected(@NonNull BookmarkCategory category)
  {
    BookmarkManager.INSTANCE.deleteCategory(category.getId());
    getAdapter().notifyDataSetChanged();
    onDeleteActionSelected();
  }

  protected void onDeleteActionSelected()
  {
    // Do nothing.
  }

  protected void onActivityResultInternal(int requestCode, int resultCode, @NonNull Intent data)
  {
    // Do nothing.
  }

  @Override
  public final void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (resultCode == Activity.RESULT_OK && requestCode == REQ_CODE_DELETE_CATEGORY)
    {
      onDeleteActionSelected(getSelectedCategory());
      return;
    }
    onActivityResultInternal(requestCode, resultCode, data);
  }

  @Override
  public void onItemLongClick(@NonNull View v, @NonNull BookmarkCategory category)
  {
    showBottomMenu(category);
  }

  static void setEnableForMenuItem(@IdRes int id, @NonNull BottomSheet bottomSheet,
                                   boolean enable)
  {
    BottomSheetHelper
        .findItemById(bottomSheet, id)
        .setVisible(enable)
        .setEnabled(enable);
  }

  private void onSaveText(@NonNull String text)
  {
    if (mCategoryEditor != null)
      mCategoryEditor.commit(text);

    getAdapter().notifyDataSetChanged();
  }

  @NonNull
  BookmarkManager.BookmarksCatalogListener createCatalogListener()
  {
    return new BookmarkManager.DefaultBookmarksCatalogListener();
  }

  @NonNull
  protected BookmarkCategory getSelectedCategory()
  {
    if (mSelectedCategory == null)
      throw new AssertionError("Invalid attempt to use null selected category.");
    return mSelectedCategory;
  }

  interface CategoryEditor
  {
    void commit(@NonNull String newName);
  }


  protected enum MenuItemClickProcessorWrapper
  {
    SET_SHARE(R.id.share, shareAction(), new Analytics(Statistics.ParamValue.SEND_AS_FILE)),
    SET_EDIT(R.id.edit, editAction(), new Analytics(Statistics.ParamValue.EDIT)),
    SHOW_ON_MAP(R.id.show_on_map, showAction(), new Analytics(Statistics.ParamValue.MAKE_INVISIBLE_ON_MAP)),
    SHARING_OPTIONS(R.id.sharing_options, showSharingOptions(), new Analytics(Statistics.ParamValue.SHARING_OPTIONS)),
    LIST_SETTINGS(R.id.settings, showListSettings(), new Analytics(Statistics.ParamValue.LIST_SETTINGS)),
    DELETE_LIST(R.id.delete, deleteAction(), new Analytics(Statistics.ParamValue.DELETE_GROUP));

    @NonNull
    private static MenuClickProcessorBase showSharingOptions()
    {
      return new MenuClickProcessorBase.OpenSharingOptions();
    }

    @NonNull
    private static MenuClickProcessorBase showListSettings()
    {
      return new MenuClickProcessorBase.OpenListSettings();
    }

    @NonNull
    private static MenuClickProcessorBase.ShowAction showAction()
    {
      return new MenuClickProcessorBase.ShowAction();
    }

    @NonNull
    private static MenuClickProcessorBase.ShareAction shareAction()
    {
      return new MenuClickProcessorBase.ShareAction();
    }

    @NonNull
    private static MenuClickProcessorBase.DeleteAction deleteAction()
    {
      return new MenuClickProcessorBase.DeleteAction();
    }

    @NonNull
    private static MenuClickProcessorBase.EditAction editAction()
    {
      return new MenuClickProcessorBase.EditAction();
    }

    @IdRes
    private final int mId;
    @NonNull
    private MenuClickProcessorBase mInternalProcessor;
    @NonNull
    private final Analytics mAnalytics;

    MenuItemClickProcessorWrapper(@IdRes int id, @NonNull MenuClickProcessorBase processorBase,
                                  @NonNull Analytics analytics)
    {
      mId = id;
      mInternalProcessor = processorBase;
      mAnalytics = analytics;
    }

    @NonNull
    public static MenuItemClickProcessorWrapper getInstance(@IdRes int resId)
    {
      for (MenuItemClickProcessorWrapper each : values())
      {
        if (each.mId == resId)
        {
          return each;
        }
      }
      throw new IllegalArgumentException("Enum value for res id = " + resId + " not found");
    }

    @NonNull
    public Analytics getAnalytics()
    {
      return mAnalytics;
    }
  }

  protected static abstract class MenuClickProcessorBase
  {
    public abstract void process(@NonNull BaseBookmarkCategoriesFragment frag,
                                 @NonNull BookmarkCategory category);

    protected static class ShowAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BaseBookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        BookmarkManager.INSTANCE.toggleCategoryVisibility(category);
        frag.getAdapter().notifyDataSetChanged();
      }
    }

    protected static class ShareAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BaseBookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        frag.onShareActionSelected(category);
      }
    }

    protected static class DeleteAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BaseBookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        frag.onDeleteActionSelected(category);
      }
    }

    protected static class EditAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BaseBookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        frag.mCategoryEditor = newName ->
        {
          BookmarkManager.INSTANCE.setCategoryName(category.getId(), newName);
        };
        EditTextDialogFragment.show(frag.getString(R.string.bookmark_set_name),
                                    category.getName(),
                                    frag.getString(R.string.rename),
                                    frag.getString(R.string.cancel),
                                    MAX_CATEGORY_NAME_LENGTH,
                                    frag);
      }
    }

    protected static class OpenSharingOptions extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BaseBookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        UgcRouteSharingOptionsActivity.startForResult(frag.getActivity(), category);
      }
    }

    protected static class OpenListSettings extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BaseBookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        UgcRouteEditSettingsActivity.startForResult(frag.getActivity(), category);
      }
    }
  }

  private static class CategoriesAdapterObserver implements DataChangedListener<BaseBookmarkCategoriesFragment>
  {
    @Nullable
    private BaseBookmarkCategoriesFragment mFragment;

    CategoriesAdapterObserver(@NonNull BaseBookmarkCategoriesFragment fragment)
    {
      mFragment = fragment;
    }

    @Override
    public void attach(@NonNull BaseBookmarkCategoriesFragment object)
    {
      mFragment = object;
    }

    @Override
    public void detach()
    {
      mFragment = null;
    }

    @Override
    public void onChanged()
    {
      if (mFragment == null)
        return;

      FilterStrategy strategy = mFragment.getType().getFilterStrategy();
      AbstractCategoriesSnapshot.Default snapshot =
          BookmarkManager.INSTANCE.getCategoriesSnapshot(strategy);
      mFragment.getAdapter().setItems(snapshot.getItems());
    }
  }
}
