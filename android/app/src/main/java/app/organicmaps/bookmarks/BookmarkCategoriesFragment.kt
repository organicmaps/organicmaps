package app.organicmaps.bookmarks

import android.os.Handler
import android.os.Looper
import android.app.Activity
import android.app.Dialog
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
import app.organicmaps.util.bottomsheet.create as exportMenuItems
import app.organicmaps.widget.recycler.CardSectionDividerDecoration
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import java.io.File

class BookmarkCategoriesFragment :
    BaseMwmRecyclerFragment<BookmarkCategoriesAdapter>(),
    BookmarkManager.BookmarksLoadingListener,
    CategoryListCallback,
    MenuBottomSheetFragment.MenuBottomSheetInterface {

    private val mainHandler = Handler(Looper.getMainLooper())

    private var skipExistingOnImport = false

    private var selectedCategory: BookmarkCategory? = null

    private var importDialog: Dialog? = null

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

    @LayoutRes
    override fun getLayoutRes(): Int = R.layout.fragment_bookmark_categories

    override fun createAdapter(): BookmarkCategoriesAdapter = BookmarkCategoriesAdapter(
        categories = BookmarkManager.INSTANCE.categories,
        onCategoryClick = ::openCategory,
        onCategoryMenuClick = ::showBottomMenu,
        categoryListCallback = this,
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        savedInstanceState?.let {
            skipExistingOnImport = it.getBoolean(EXTRA_SKIP_EXISTING_ON_IMPORT)
            selectedCategory = Utils.getParcelable(it, EXTRA_SELECTED_CATEGORY, BookmarkCategory::class.java)
        }
        (childFragmentManager.findFragmentByTag(EditTextDialogFragment.TAG) as EditTextDialogFragment?)
            ?.setupNewListDialog()
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
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
        outState.putBoolean(EXTRA_SKIP_EXISTING_ON_IMPORT, skipExistingOnImport)
        selectedCategory?.let { outState.putParcelable(EXTRA_SELECTED_CATEGORY, it) }
    }

    private fun openCategory(category: BookmarkCategory) {
        selectedCategory = category
        BookmarkListActivity.startForResult(this, startBookmarkListForResult, category)
    }

    private fun showBottomMenu(category: BookmarkCategory) {
        selectedCategory = category
        MenuBottomSheetFragment.newInstance(BOOKMARKS_CATEGORIES_MENU_ID, category.name)
            .show(childFragmentManager, BOOKMARKS_CATEGORIES_MENU_ID)
    }

    override fun getMenuBottomSheetItems(id: String): ArrayList<MenuBottomSheetItem> {
        val category = requireSelectedCategory()
        return ArrayList<MenuBottomSheetItem>().apply {
            add(MenuBottomSheetItem(R.string.edit, R.drawable.ic_settings) { onSettingsActionSelected(category) })
            add(
                MenuBottomSheetItem(
                    if (category.isVisible) R.string.hide else R.string.show,
                    if (category.isVisible) R.drawable.ic_hide else R.drawable.ic_show,
                ) { onShowActionSelected(category) },
            )
            addAll(
                exportMenuItems(hasAreas = category.areasCount > 0) { fileType ->
                    onShareActionSelected(category, fileType)
                },
            )
            // Disallow deleting the last category
            if (adapter.bookmarkCategories.size > 1) {
                add(MenuBottomSheetItem(R.string.delete, R.drawable.ic_delete) { onDeleteActionSelected(category) })
            }
        }
    }

    override fun onAddButtonClick() {
        EditTextDialogFragment
            .show(
                getString(R.string.bookmarks_create_new_group),
                getString(R.string.bookmarks_new_list_hint),
                getString(R.string.bookmark_set_name),
                getString(R.string.create),
                getString(R.string.cancel),
                CategoryValidator.MAX_NAME_LENGTH,
                this,
            ).setupNewListDialog()
    }

    private fun EditTextDialogFragment.setupNewListDialog() {
        setValidator(CategoryValidator())
        setTextSaveListener { BookmarkManager.INSTANCE.createCategory(it) }
    }

    override fun onImportButtonClick() {
        var skipExisting = skipExistingOnImport
        MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setTitle(R.string.bookmarks_import)
            .setMultiChoiceItems(
                arrayOf(getString(R.string.bookmarks_import_skip_existing)),
                booleanArrayOf(skipExisting),
            ) { _, _, checked -> skipExisting = checked }
            .setPositiveButton(android.R.string.ok) { _, _ ->
                skipExistingOnImport = skipExisting
                launchImportPicker()
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun launchImportPicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE)
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
        startActivity(
            Intent(requireActivity(), BookmarkCategorySettingsActivity::class.java)
                .putExtra(BookmarkCategorySettingsActivity.EXTRA_BOOKMARK_CATEGORY, category),
        )
    }

    private fun onShowActionSelected(category: BookmarkCategory) = category.toggleVisibility()

    private fun onDeleteActionSelected(category: BookmarkCategory) {
        BookmarkManager.INSTANCE.deleteCategory(category.id)
    }

    override fun onDeleteAllButtonClick() {
        val totals = currentTotals()
        MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setTitle(R.string.bookmarks_delete_all)
            .setMessage(
                getString(R.string.bookmarks_delete_all_confirm, totals.lists, totals.bookmarks, totals.tracks),
            )
            .setPositiveButton(R.string.delete) { _, _ ->
                val resIdCoreMyPlaces = app.organicmaps.sdk.R.string.core_my_places
                BookmarkManager.INSTANCE.deleteAllCategories(getString(resIdCoreMyPlaces))
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun showNoFileManagerError() {
        MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setMessage(R.string.error_no_file_manager_app)
            .setPositiveButton(android.R.string.ok) { dialog, _ -> dialog.dismiss() }
            .show()
    }

    private data class Totals(val lists: Int, val bookmarks: Int, val tracks: Int)

    private fun currentTotals(): Totals {
        val categories = BookmarkManager.INSTANCE.categories
        return Totals(categories.size, categories.sumOf { it.bookmarksCount }, categories.sumOf { it.tracksCount })
    }

    private fun importBookmarks(rootUri: Uri) {
        val appContext: Context = requireContext().applicationContext
        importDialog = showImportDialog()

        Logger.d(TAG, "Importing bookmarks from $rootUri, skipExisting=$skipExistingOnImport")
        val manager = BookmarkManager.INSTANCE
        val tempDir = File(StorageUtils.getTempPath(MwmApplication.from(appContext)))
        val resolver = appContext.contentResolver
        // Collected on the UI thread: the list is not thread-safe, and the worker only touches this copy.
        val knownNames: MutableSet<String>? =
            if (skipExistingOnImport) {
                manager.categories.mapTo(HashSet()) { BookmarkManager.normalizeListName(it.name) }
            } else {
                null
            }

        // All state below is only touched on the UI thread.
        var expectedFiles = -1 // number of files handed to the core, known once the worker is done
        var loadedFiles = 0
        var skippedFiles = 0
        var finished = false
        lateinit var loadingListener: BookmarkManager.BookmarksLoadingListener

        fun finish() {
            if (finished) return
            finished = true
            manager.removeLoadingListener(loadingListener)
            dismissImportDialog()
            val found = expectedFiles.coerceAtLeast(0)
            val totals = currentTotals()
            val message = buildString {
                append(appContext.resources.getQuantityString(R.plurals.bookmarks_detect_message, found, found))
                if (skippedFiles > 0) {
                    append('\n').append(appContext.getString(R.string.bookmarks_import_skipped, skippedFiles))
                }
                append('\n').append(
                    appContext.getString(R.string.bookmarks_import_totals, totals.lists, totals.bookmarks, totals.tracks),
                )
            }
            Toast.makeText(appContext, message, Toast.LENGTH_LONG).show()
        }

        fun finishIfLoaded() {
            // Posted: the core is iterating over the listeners right now, and the lists cache is updated by then.
            if (expectedFiles >= 0 && loadedFiles >= expectedFiles) mainHandler.post { finish() }
        }

        loadingListener = object : BookmarkManager.BookmarksLoadingListener {
            override fun onBookmarksFileImportSuccessful() {
                loadedFiles++
                finishIfLoaded()
            }

            override fun onBookmarksFileImportFailed() {
                loadedFiles++
                finishIfLoaded()
            }
        }
        manager.addLoadingListener(loadingListener)

        ThreadPool.getStorage().execute {
            var found = 0
            var skipped = 0
            StorageUtils.listContentProviderFilesRecursively(resolver, rootUri) { uri ->
                when (manager.importBookmarksFile(resolver, uri, tempDir, knownNames)) {
                    BookmarkManager.ImportResult.IMPORTED -> found++
                    BookmarkManager.ImportResult.SKIPPED -> skipped++
                    BookmarkManager.ImportResult.FAILED -> Unit
                }
            }
            UiThread.run {
                expectedFiles = found
                skippedFiles = skipped
                if (found == 0) {
                    finish()
                } else {
                    finishIfLoaded()
                    // Safety net: never leave the progress dialog up if a callback gets lost.
                    mainHandler.postDelayed({ finish() }, IMPORT_TIMEOUT_MS)
                }
            }
        }
    }

    @Suppress("DEPRECATION")
    private fun showImportDialog(): Dialog = ProgressDialog(requireActivity(), R.style.MwmTheme_ProgressDialog).apply {
        setMessage(getString(R.string.wait_several_minutes))
        setProgressStyle(ProgressDialog.STYLE_SPINNER)
        isIndeterminate = true
        setCancelable(false)
        show()
    }

    private fun dismissImportDialog() {
        importDialog?.dismiss()
        importDialog = null
    }

    override fun onBookmarksFileImportFailed() {
        Utils.showSnackbar(requireActivity(), requireView(), R.string.load_kmz_failed)
    }

    private fun requireSelectedCategory(): BookmarkCategory =
        checkNotNull(selectedCategory) { "Invalid attempt to use null selected category." }

    private companion object {
        val TAG: String = BookmarkCategoriesFragment::class.java.simpleName
        const val BOOKMARKS_CATEGORIES_MENU_ID = "BOOKMARKS_CATEGORIES_BOTTOM_SHEET"
        const val EXTRA_SELECTED_CATEGORY = "selected_category"
        const val EXTRA_SKIP_EXISTING_ON_IMPORT = "skip_existing_on_import"
        const val IMPORT_TIMEOUT_MS = 30_000L
    }
}
