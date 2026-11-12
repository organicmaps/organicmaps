package app.organicmaps.bookmarks

import android.Manifest
import android.app.Activity
import android.app.Dialog
import android.app.ProgressDialog
import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.DocumentsContract
import android.provider.Settings
import android.view.View
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.LayoutRes
import androidx.appcompat.app.AlertDialog
import androidx.core.content.ContextCompat
import app.organicmaps.MwmApplication
import app.organicmaps.R
import app.organicmaps.base.BaseMwmRecyclerFragment
import app.organicmaps.contacts.ContactsImportCoordinator
import app.organicmaps.contacts.ContactsImporter
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

    private var selectedCategory: BookmarkCategory? = null

    private var importDialog: Dialog? = null
    private var contactsDialog: AlertDialog? = null
    private var contactsProgressHidden = false
    private lateinit var contactsImport: ContactsImportCoordinator

    private val contactsNotificationPermission =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) {
            contactsImport.start()
        }

    private fun startContactsImport() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.POST_NOTIFICATIONS) !=
            PackageManager.PERMISSION_GRANTED
        ) {
            contactsNotificationPermission.launch(Manifest.permission.POST_NOTIFICATIONS)
        } else {
            contactsImport.start()
        }
    }

    private val contactsPermission = registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
        if (granted) {
            startContactsImport()
        } else {
            MaterialAlertDialogBuilder(requireContext())
                .setMessage(R.string.bookmarks_contacts_permission_denied)
                .setNegativeButton(R.string.cancel, null)
                .setPositiveButton(R.string.settings) { _, _ ->
                    try {
                        startActivity(
                            Intent(
                                Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                                Uri.fromParts("package", requireContext().packageName, null),
                            ),
                        )
                    } catch (_: ActivityNotFoundException) {
                        Toast.makeText(
                            requireContext(),
                            R.string.bookmarks_contacts_permission_denied,
                            Toast.LENGTH_LONG,
                        ).show()
                    }
                }.show()
        }
    }

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
            selectedCategory = Utils.getParcelable(it, EXTRA_SELECTED_CATEGORY, BookmarkCategory::class.java)
        }
        (childFragmentManager.findFragmentByTag(EditTextDialogFragment.TAG) as EditTextDialogFragment?)
            ?.setupNewListDialog()
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        recyclerView.addItemDecoration(CardSectionDividerDecoration(requireContext()))
        BookmarkManager.INSTANCE.addCategoriesUpdatesListener(categoriesUpdatesListener)
        contactsImport = ContactsImportCoordinator.getInstance(requireActivity().application)
        contactsImport.state.observe(viewLifecycleOwner, ::showContactsImportState)
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
        contactsDialog?.dismiss()
        contactsDialog = null
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
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

    override fun onImportContactsButtonClick() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return
        contactsProgressHidden = false
        if (contactsImport.wasInterrupted()) {
            MaterialAlertDialogBuilder(requireContext())
                .setMessage(R.string.bookmarks_contacts_import_interrupted)
                .setNegativeButton(R.string.cancel, null)
                .setPositiveButton(R.string.bookmarks_import_contacts) { _, _ -> startContactsImport() }
                .show()
            return
        }
        val current = contactsImport.state.value
        if (current != null && current.isRunning) {
            showContactsImportState(current)
            return
        }
        MaterialAlertDialogBuilder(requireContext())
            .setTitle(R.string.bookmarks_import_contacts)
            .setMessage(R.string.bookmarks_contacts_import_explanation)
            .setNegativeButton(R.string.cancel, null)
            .setPositiveButton(R.string.bookmarks_import_contacts) { _, _ ->
                if (ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.READ_CONTACTS)
                    == PackageManager.PERMISSION_GRANTED
                ) {
                    startContactsImport()
                } else {
                    contactsPermission.launch(Manifest.permission.READ_CONTACTS)
                }
            }.show()
    }

    private fun showContactsImportState(state: ContactsImporter.State) {
        adapter.contactImportProgress = if (state.isRunning) {
            if (state.total > 0) {
                getString(R.string.bookmarks_contacts_import_resolving, state.processed, state.total)
            } else {
                getString(R.string.bookmarks_contacts_import_reading)
            }
        } else {
            null
        }
        if (state.isRunning) {
            if (contactsProgressHidden) return
            if (contactsDialog == null) {
                contactsDialog = MaterialAlertDialogBuilder(requireContext())
                    .setTitle(R.string.bookmarks_import_contacts)
                    .setView(R.layout.dialog_contacts_import)
                    .setNegativeButton(R.string.cancel) { _, _ -> contactsImport.cancel() }
                    .setNeutralButton(R.string.hide) { _, _ ->
                        contactsProgressHidden = true
                        contactsDialog = null
                    }
                    .setOnCancelListener { contactsImport.cancel() }
                    .create()
                contactsDialog?.show()
                contactsDialog?.setCanceledOnTouchOutside(false)
            }
            val message = when (state.phase) {
                ContactsImporter.Phase.READING -> getString(R.string.bookmarks_contacts_import_reading)
                ContactsImporter.Phase.SAVING -> getString(R.string.bookmarks_contacts_import_saving)
                else -> getString(R.string.bookmarks_contacts_import_resolving, state.processed, state.total)
            }
            contactsDialog?.findViewById<android.widget.TextView>(R.id.contacts_import_progress)?.text = message
            contactsDialog?.setCancelable(state.phase != ContactsImporter.Phase.SAVING)
            contactsDialog?.getButton(AlertDialog.BUTTON_NEGATIVE)?.isEnabled =
                state.phase != ContactsImporter.Phase.SAVING
            return
        }
        contactsDialog?.dismiss()
        contactsDialog = null
        val message = when (state.phase) {
            ContactsImporter.Phase.DONE -> {
                val result = checkNotNull(state.result)
                if (state.total == 0) {
                    getString(R.string.bookmarks_contacts_import_empty)
                } else {
                    getString(
                        R.string.bookmarks_contacts_import_result,
                        result.added,
                        result.updated,
                        result.unchanged,
                        result.edited,
                        result.skipped + state.unresolved,
                    )
                }
            }

            ContactsImporter.Phase.ERROR -> getString(
                when (state.error) {
                    ContactsImporter.Error.PERMISSION -> R.string.bookmarks_contacts_permission_denied
                    ContactsImporter.Error.MAPS -> R.string.bookmarks_contacts_import_maps
                    ContactsImporter.Error.READ -> R.string.bookmarks_contacts_import_read_error
                    else -> R.string.bookmarks_contacts_import_save_error
                },
            )

            ContactsImporter.Phase.CANCELLED -> {
                contactsImport.dismissResult()
                return
            }

            else -> return
        }
        contactsDialog = MaterialAlertDialogBuilder(requireContext())
            .setTitle(R.string.bookmarks_import_contacts)
            .setMessage(message)
            .setPositiveButton(android.R.string.ok) { _, _ -> contactsImport.dismissResult() }
            .apply {
                val category = state.result?.let { result ->
                    BookmarkManager.INSTANCE.categories.firstOrNull { it.id == result.categoryId }
                }
                if (category != null) {
                    setNeutralButton(R.string.bookmarks_contacts_list_name) { _, _ ->
                        contactsImport.dismissResult()
                        openCategory(category)
                    }
                }
            }
            .setOnCancelListener { contactsImport.dismissResult() }
            .show()
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

    private fun showNoFileManagerError() {
        MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setMessage(R.string.error_no_file_manager_app)
            .setPositiveButton(android.R.string.ok) { dialog, _ -> dialog.dismiss() }
            .show()
    }

    private fun importBookmarks(rootUri: Uri) {
        val appContext: Context = requireContext().applicationContext
        importDialog = showImportDialog()

        Logger.d(TAG, "Importing bookmarks from $rootUri")
        val tempDir = File(StorageUtils.getTempPath(MwmApplication.from(appContext)))
        val resolver = appContext.contentResolver
        ThreadPool.getStorage().execute {
            var found = 0
            StorageUtils.listContentProviderFilesRecursively(resolver, rootUri) { uri ->
                if (BookmarkManager.INSTANCE.importBookmarksFile(resolver, uri, tempDir)) {
                    found++
                }
            }
            UiThread.run {
                dismissImportDialog()
                val message = appContext.resources.getQuantityString(R.plurals.bookmarks_detect_message, found, found)
                Toast.makeText(appContext, message, Toast.LENGTH_LONG).show()
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
    }
}
