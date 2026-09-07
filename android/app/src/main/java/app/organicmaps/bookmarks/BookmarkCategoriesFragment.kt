package app.organicmaps.bookmarks

import android.app.Activity
import android.app.ProgressDialog
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.DocumentsContract
import android.view.View
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.LayoutRes
import app.organicmaps.MwmApplication
import app.organicmaps.R
import app.organicmaps.adapter.OnItemClickListener
import app.organicmaps.base.BaseMwmRecyclerFragment
import app.organicmaps.dialog.EditTextDialogFragment
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
import app.organicmaps.sdk.bookmarks.data.DataChangedListener
import app.organicmaps.sdk.bookmarks.data.FileType
import app.organicmaps.sdk.util.StorageUtils
import app.organicmaps.sdk.util.concurrency.ThreadPool
import app.organicmaps.sdk.util.concurrency.UiThread
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.util.SharingUtils
import app.organicmaps.util.Utils
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem
import app.organicmaps.util.bottomsheet.create
import app.organicmaps.widget.PlaceholderView
import app.organicmaps.widget.recycler.CardSectionDividerDecoration
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import java.io.File
import java.util.concurrent.atomic.AtomicInteger

class BookmarkCategoriesFragment :
    BaseMwmRecyclerFragment<BookmarkCategoriesAdapter>(),
    BookmarkManager.BookmarksLoadingListener,
    CategoryListCallback,
    OnItemClickListener<BookmarkCategory>,
    OnItemMoreClickListener<BookmarkCategory>,
    OnItemLongClickListener<BookmarkCategory>,
    MenuBottomSheetFragment.MenuBottomSheetInterface {

    private var selectedCategory: BookmarkCategory? = null

    /** What a name typed in the "new list" dialog is committed to. */
    private var categoryEditor: ((String) -> Unit)? = null

    @Suppress("DEPRECATION")
    private var importDialog: ProgressDialog? = null

    private val categoriesUpdatesListener = DataChangedListener {
        adapter.setItems(BookmarkManager.INSTANCE.categories)
    }

    private val shareLauncher = SharingUtils.RegisterLauncher(this)

    private val startBookmarkListForResult =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
            if (result.resultCode == Activity.RESULT_OK) {
                onDeleteActionSelected(requireSelectedCategory())
            }
        }

    private val startImportDirectoryForResult =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
            if (result.resultCode == Activity.RESULT_OK) {
                importBookmarks(checkNotNull(result.data?.data) { "No directory returned by the picker" })
            }
        }

    private val startBookmarkSettingsForResult =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
            // not handled at the moment
        }

    @LayoutRes
    override fun getLayoutRes(): Int = R.layout.fragment_bookmark_categories

    override fun createAdapter(): BookmarkCategoriesAdapter =
        BookmarkCategoriesAdapter(requireContext(), BookmarkManager.INSTANCE.categories)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        savedInstanceState?.let {
            selectedCategory = Utils.getParcelable(it, EXTRA_SELECTED_CATEGORY, BookmarkCategory::class.java)
        }

        adapter.onClickListener = this
        adapter.onLongClickListener = this
        adapter.onMoreClickListener = this
        adapter.categoryListCallback = this

        // The recycler is the root of the layout and fills it, so its own size never follows the content.
        recyclerView.setHasFixedSize(true)
        recyclerView.addItemDecoration(CardSectionDividerDecoration(requireContext()))
        BookmarkManager.INSTANCE.addCategoriesUpdatesListener(categoriesUpdatesListener)
    }

    override fun onStart() {
        super.onStart()
        BookmarkManager.INSTANCE.addLoadingListener(this)
    }

    override fun onStop() {
        super.onStop()
        BookmarkManager.INSTANCE.removeLoadingListener(this)
    }

    override fun onDestroyView() {
        super.onDestroyView()
        BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(categoriesUpdatesListener)
        // The import keeps running, but its dialog belongs to the Activity that is going away.
        dismissImportDialog()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        selectedCategory?.let { outState.putParcelable(EXTRA_SELECTED_CATEGORY, it) }
    }

    override fun setupPlaceholder(placeholder: PlaceholderView?) {
        // A placeholder is not needed on this screen.
    }

    // Rows

    override fun onItemClick(v: View, category: BookmarkCategory) {
        selectedCategory = category
        BookmarkListActivity.startForResult(this, startBookmarkListForResult, category)
    }

    override fun onItemLongClick(v: View, category: BookmarkCategory) = showBottomMenu(category)

    override fun onItemMoreClick(v: View, category: BookmarkCategory) = showBottomMenu(category)

    private fun showBottomMenu(category: BookmarkCategory) {
        selectedCategory = category
        MenuBottomSheetFragment.newInstance(BOOKMARKS_CATEGORIES_MENU_ID, category.name)
            .show(childFragmentManager, BOOKMARKS_CATEGORIES_MENU_ID)
    }

    /** Unreachable in practice: the sheet is only shown from showBottomMenu, which selects a category first. */
    override fun getMenuBottomSheetItems(id: String): ArrayList<MenuBottomSheetItem>? {
        val category = selectedCategory ?: return null
        return ArrayList<MenuBottomSheetItem>().apply {
            add(MenuBottomSheetItem(R.string.edit, R.drawable.ic_settings) { onSettingsActionSelected(category) })
            add(
                MenuBottomSheetItem(
                    if (category.isVisible) R.string.hide else R.string.show,
                    if (category.isVisible) R.drawable.ic_hide else R.drawable.ic_show,
                ) { onShowActionSelected(category) },
            )
            addAll(create { fileType -> onShareActionSelected(category, fileType) })
            // Disallow deleting the last category
            if (adapter.bookmarkCategories.size > 1) {
                add(MenuBottomSheetItem(R.string.delete, R.drawable.ic_delete) { onDeleteActionSelected(category) })
            }
        }
    }

    // Actions

    override fun onAddButtonClick() {
        categoryEditor = { name -> BookmarkManager.INSTANCE.createCategory(name) }

        EditTextDialogFragment
            .show(
                getString(R.string.bookmarks_create_new_group),
                getString(R.string.bookmarks_new_list_hint),
                getString(R.string.bookmark_set_name),
                getString(R.string.create),
                getString(R.string.cancel),
                CategoryValidator.MAX_NAME_LENGTH,
                this,
            ).apply {
                setValidator(CategoryValidator())
                setTextSaveListener(::onSaveText)
            }
    }

    override fun onImportButtonClick() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE)
            // Sic: EXTRA_INITIAL_URI doesn't work
            // https://stackoverflow.com/questions/65326605/extra-initial-uri-will-not-work-no-matter-what-i-do
            // Enable "Show SD card option", http://stackoverflow.com/a/31334967/1615876
            .putExtra("android.content.extra.SHOW_ADVANCED", true)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            intent.putExtra(DocumentsContract.EXTRA_EXCLUDE_SELF, true)
        }

        if (intent.resolveActivity(requireActivity().packageManager) != null) {
            startImportDirectoryForResult.launch(intent)
        } else {
            showNoFileManagerError()
        }
    }

    override fun onExportButtonClick() {
        BookmarksSharingHelper.INSTANCE.prepareBookmarkCategoriesForSharing(requireActivity(), shareLauncher)
    }

    private fun onShareActionSelected(category: BookmarkCategory, fileType: FileType) {
        BookmarksSharingHelper.INSTANCE
            .prepareBookmarkCategoryForSharing(requireActivity(), shareLauncher, category.id, fileType)
    }

    private fun onSettingsActionSelected(category: BookmarkCategory) {
        BookmarkCategorySettingsActivity.startForResult(this, startBookmarkSettingsForResult, category)
    }

    private fun onShowActionSelected(category: BookmarkCategory) = category.toggleVisibility()

    private fun onDeleteActionSelected(category: BookmarkCategory) {
        BookmarkManager.INSTANCE.deleteCategory(category.id)
        // The snapshot now names a category the core has dropped, and every accessor on it aborts in native code.
        selectedCategory = null
    }

    private fun onSaveText(text: String) {
        categoryEditor?.invoke(text)
    }

    private fun showNoFileManagerError() {
        MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setMessage(R.string.error_no_file_manager_app)
            .setPositiveButton(android.R.string.ok) { dialog, _ -> dialog.dismiss() }
            .show()
    }

    // Import

    /**
     * The scan walks the whole directory tree and can run for minutes, so the worker must outlive the Activity
     * without holding it: it keeps the application context, and the dialog is owned by the fragment instead.
     */
    @Suppress("DEPRECATION")
    private fun importBookmarks(rootUri: Uri) {
        val appContext: Context = requireContext().applicationContext
        importDialog = ProgressDialog(requireActivity(), R.style.MwmTheme_ProgressDialog).apply {
            setMessage(getString(R.string.wait_several_minutes))
            setProgressStyle(ProgressDialog.STYLE_SPINNER)
            isIndeterminate = true
            setCancelable(false)
            show()
        }

        Logger.d(TAG, "Importing bookmarks from $rootUri")
        val tempDir = File(StorageUtils.getTempPath(MwmApplication.from(appContext)))
        val resolver = appContext.contentResolver
        ThreadPool.getStorage().execute {
            val found = AtomicInteger(0)
            StorageUtils.listContentProviderFilesRecursively(resolver, rootUri) { uri ->
                if (BookmarkManager.INSTANCE.importBookmarksFile(resolver, uri, tempDir)) {
                    found.incrementAndGet()
                }
            }
            UiThread.run {
                dismissImportDialog()
                val count = found.get()
                val message = appContext.resources.getQuantityString(R.plurals.bookmarks_detect_message, count, count)
                Toast.makeText(appContext, message, Toast.LENGTH_LONG).show()
            }
        }
    }

    private fun dismissImportDialog() {
        importDialog?.takeIf { it.isShowing }?.dismiss()
        importDialog = null
    }

    // Bookmark loading

    override fun onBookmarksLoadingFinished() {
        // The core refreshes its category cache before dispatching this, so the update listener has already run.
    }

    /**
     * Reports a single failure without naming the file: the callback carries neither the name nor a way to
     * queue several notifications. The snackbar is anchored to the fragment view rather than to the import row.
     */
    override fun onBookmarksFileImportFailed() {
        view?.let { Utils.showSnackbar(requireActivity(), it, R.string.load_kmz_failed) }
    }

    private fun requireSelectedCategory(): BookmarkCategory =
        checkNotNull(selectedCategory) { "Invalid attempt to use null selected category." }

    private companion object {
        val TAG: String = BookmarkCategoriesFragment::class.java.simpleName
        const val BOOKMARKS_CATEGORIES_MENU_ID = "BOOKMARKS_CATEGORIES_BOTTOM_SHEET"
        const val EXTRA_SELECTED_CATEGORY = "selected_category"
    }
}
