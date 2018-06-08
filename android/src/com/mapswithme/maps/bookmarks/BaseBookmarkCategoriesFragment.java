package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.IdRes;
import android.support.annotation.LayoutRes;
import android.support.annotation.MenuRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.MenuItem;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.BookmarkSharingResult;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.maps.widget.PlaceholderView;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.sharing.SharingHelper;

public abstract class BaseBookmarkCategoriesFragment extends BaseMwmRecyclerFragment
    implements EditTextDialogFragment.EditTextDialogInterface,
               MenuItem.OnMenuItemClickListener,
               BookmarkManager.BookmarksLoadingListener,
               BookmarkManager.BookmarksSharingListener,
               CategoryListCallback,
               KmlImportController.ImportKmlCallback,
               OnItemClickListener<BookmarkCategory>,
               OnItemLongClickListener<BookmarkCategory>

{
  private static final int MAX_CATEGORY_NAME_LENGTH = 60;
  @NonNull
  private BookmarkCategory mSelectedCategory;
  @Nullable
  private CategoryEditor mCategoryEditor;
  @Nullable
  private KmlImportController mKmlImportController;
  @NonNull
  private Runnable mImportKmlTask = new ImportKmlTask();

  @Override
  @LayoutRes
  protected int getLayoutRes()
  {
    return R.layout.fragment_bookmark_categories;
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new BookmarkCategoriesAdapter(getActivity());
  }

  @Nullable
  @Override
  protected BookmarkCategoriesAdapter getAdapter()
  {
    RecyclerView.Adapter adapter = super.getAdapter();
    return adapter != null ? (BookmarkCategoriesAdapter) adapter : null;
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    onPrepareControllers(view);
    if (getAdapter() != null)
    {
      getAdapter().setOnClickListener(this);
      getAdapter().setOnLongClickListener(this);
      getAdapter().setCategoryListCallback(this);
    }

    RecyclerView rw = getRecyclerView();
    if (rw == null) return;

    rw.setNestedScrollingEnabled(false);
    RecyclerView.ItemDecoration decor = ItemDecoratorFactory
        .createVerticalDefaultDecorator(getContext());
    rw.addItemDecoration(decor);
  }

  protected void onPrepareControllers(@NonNull View view)
  {
    mKmlImportController = new KmlImportController(getActivity(), this);
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
    BookmarkManager.INSTANCE.addSharingListener(this);
    if (mKmlImportController != null)
      mKmlImportController.onStart();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeLoadingListener(this);
    BookmarkManager.INSTANCE.removeSharingListener(this);
    if (mKmlImportController != null)
      mKmlImportController.onStop();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateLoadingPlaceholder();
    if (getAdapter() != null)
      getAdapter().notifyDataSetChanged();
    if (!BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress())
      mImportKmlTask.run();
  }

  @Override
  public void onPause()
  {
    super.onPause();
  }

  @Override
  public boolean onMenuItemClick(MenuItem item)
  {
    MenuItemClickProcessorWrapper processor = MenuItemClickProcessorWrapper
        .getInstance(item.getItemId());
    processor
        .mInternalProcessor
        .process(this, mSelectedCategory);
    return true;
  }


  protected final void showBottomMenu(@NonNull BookmarkCategory item)
  {
    mSelectedCategory = item;
    showBottomMenuInternal(item);
  }

  protected void showBottomMenuInternal(@NonNull BookmarkCategory item)
  {
    BottomSheetHelper.Builder bs = BottomSheetHelper.create(getActivity(), item.getName())
                                                    .sheet(getCategoryMenuResId())
                                                    .listener(this);

    bs.getItemByIndex(0)
      .setIcon(item.isVisible() ? R.drawable.ic_hide : R.drawable.ic_show)
      .setTitle(item.isVisible() ? R.string.hide : R.string.show);

    final boolean deleteIsPossible = getAdapter().getBookmarkCategories().size() > 1;
    bs.getItemById(getDeleteMenuItemResId())
      .setVisible(deleteIsPossible)
      .setEnabled(deleteIsPossible);

    bs.tint().show();
  }

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
    if (getAdapter() != null)
      getAdapter().notifyDataSetChanged();
    mImportKmlTask.run();
  }

  @Override
  public void onBookmarksFileLoaded(boolean success)
  {
    // Do nothing here.
  }

  private void importKml()
  {
    if (mKmlImportController != null)
      mKmlImportController.importKml();
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    SharingHelper.INSTANCE.onPreparedFileForSharing(getActivity(), result);
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
    if (getAdapter() != null) getAdapter().notifyDataSetChanged();
  }

  @NonNull
  @Override
  public EditTextDialogFragment.OnTextSaveListener getSaveTextListener()
  {
    return text -> {
      if (mCategoryEditor != null)
        mCategoryEditor.commit(text);

      if (getAdapter() != null)
        getAdapter().notifyDataSetChanged();
    };
  }

  @NonNull
  @Override
  public EditTextDialogFragment.Validator getValidator()
  {
    return new CategoryValidator();
  }

  @Override
  public void onItemClick(View v, BookmarkCategory category)
  {
    startActivity(new Intent(getActivity(), BookmarkListActivity.class)
                      .putExtra(BookmarksListFragment.EXTRA_CATEGORY, category));
  }

  @Override
  public void onItemLongClick(View v, BookmarkCategory category)
  {
    showBottomMenu(category);
  }

  @IdRes
  protected int getDeleteMenuItemResId()
  {
    return R.id.set_delete;
  }

  interface CategoryEditor
  {
    void commit(@NonNull String newName);
  }

  private class ImportKmlTask implements Runnable
  {
    private boolean alreadyDone;

    @Override
    public void run()
    {
      if (alreadyDone)
        return;

      importKml();
      alreadyDone = true;
    }
  }

  protected enum MenuItemClickProcessorWrapper
  {
    SET_SHOW(R.id.set_show, showAction()),
    SET_SHARE(R.id.set_share, shareAction()),
    SET_DELETE(R.id.set_delete, deleteAction()),
    SET_EDIT(R.id.set_edit, editAction()),
    SHOW_ON_MAP(R.id.show_on_map, showAction()),
    SHARE_LIST(R.id.share_list, shareAction()),
    DELETE_LIST(R.id.delete_list, deleteAction());

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

    MenuItemClickProcessorWrapper(@IdRes int id, @NonNull MenuClickProcessorBase processorBase)
    {
      mId = id;
      mInternalProcessor = processorBase;
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
        BookmarkManager.INSTANCE.toggleCategoryVisibility(category.getId());
        if (frag.getAdapter() != null)
          frag.getAdapter().notifyDataSetChanged();
      }
    }

    protected static class ShareAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BaseBookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        SharingHelper.INSTANCE.prepareBookmarkCategoryForSharing(frag.getActivity(),
                                                                 category.getId());
      }
    }

    protected static class DeleteAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BaseBookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        BookmarkManager.INSTANCE.deleteCategory(category.getId());
        if (frag.getAdapter() != null)
          frag.getAdapter().notifyDataSetChanged();
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
  }
}
