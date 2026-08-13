package app.organicmaps.widget.placepage

import android.content.DialogInterface
import android.content.res.Configuration
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.ImageView
import androidx.activity.result.ActivityResultLauncher
import androidx.annotation.ColorInt
import androidx.appcompat.widget.Toolbar
import androidx.core.os.BundleCompat
import androidx.core.os.bundleOf
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.isVisible
import androidx.core.widget.NestedScrollView
import androidx.fragment.app.FragmentManager
import app.organicmaps.R
import app.organicmaps.base.BaseMwmDialogFragment
import app.organicmaps.bookmarks.BookmarksSharingHelper
import app.organicmaps.bookmarks.ChooseBookmarkCategoryFragment
import app.organicmaps.dialog.DeleteConfirmationDialogFragment
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
import app.organicmaps.sdk.bookmarks.data.FileType
import app.organicmaps.util.SharingUtils
import app.organicmaps.util.UiUtils
import app.organicmaps.widget.colorpicker.ColorPickerFragment
import com.google.android.material.textfield.TextInputEditText

class EditBookmarkFragment :
    BaseMwmDialogFragment(),
    ChooseBookmarkCategoryFragment.Listener,
    ColorPickerFragment.OnColorChangeListener {

    private lateinit var etName: TextInputEditText
    private lateinit var etDescription: TextInputEditText
    private lateinit var tvBookmarkGroup: TextInputEditText
    private lateinit var ivColor: ImageView
    private lateinit var ivVisibility: ImageView
    private lateinit var nestedScrollView: NestedScrollView
    private lateinit var toolbar: Toolbar
    private lateinit var bookmarkCategory: BookmarkCategory
    private lateinit var shareLauncher: ActivityResultLauncher<SharingUtils.SharingIntent>

    private var target: EditTarget? = null
    private var deleted: Boolean = false

    private val trackTarget: TrackEditTarget? get() = target as? TrackEditTarget

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        shareLauncher = SharingUtils.RegisterLauncher(this)
    }

    override fun getCustomTheme(): Int = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
        fullscreenTheme
    } else {
        R.style.MwmTheme_DialogFragment_Fullscreen_Opaque
    }

    private fun isNightMode(): Boolean = resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK ==
        Configuration.UI_MODE_NIGHT_YES

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
        inflater.inflate(R.layout.fragment_edit_bookmark, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val args = requireArguments()

        etName = view.findViewById(R.id.et__bookmark_name)
        etDescription = view.findViewById(R.id.et__description)
        tvBookmarkGroup = view.findViewById<TextInputEditText>(R.id.tv__bookmark_set)
            .apply { setOnClickListener { selectBookmarkSet() } }
        ivColor = view.findViewById(R.id.iv__bookmark_color)
        ivVisibility = view.findViewById<ImageView>(R.id.iv__track_visibility)
            .apply { setOnClickListener { toggleVisibility() } }
        toolbar = view.findViewById(R.id.toolbar)
        nestedScrollView = view.findViewById(R.id.edit_bookmark_scroll)
        view.findViewById<View>(R.id.color_row).setOnClickListener { selectBookmarkColor() }

        target = loadTarget(args, savedInstanceState)
        bookmarkCategory = savedInstanceState?.let {
            BundleCompat.getParcelable(it, STATE_BOOKMARK_CATEGORY, BookmarkCategory::class.java)
        } ?: BookmarkManager.INSTANCE.getCategoryById(args.getLong(EXTRA_CATEGORY_ID))

        childFragmentManager.setFragmentResultListener(
            DeleteConfirmationDialogFragment.REQUEST_KEY,
            viewLifecycleOwner,
        ) { _, _ -> deleteAndDismiss() }

        bindTarget()
        initToolbar()
        setupWindowInsets(view)
    }

    private fun loadTarget(args: Bundle, savedInstanceState: Bundle?): EditTarget? {
        val id = args.getLong(EXTRA_ID)
        val kind = args.getString(EXTRA_KIND)?.let(TargetKind::valueOf) ?: return null
        val loaded: EditTarget? = when (kind) {
            TargetKind.BOOKMARK -> BookmarkManager.INSTANCE.getBookmarkInfo(id)?.let(::BookmarkEditTarget)
            TargetKind.TRACK -> BookmarkManager.INSTANCE.getTrack(id)?.let(::TrackEditTarget)
        }
        return loaded?.also { t ->
            t.color = savedInstanceState?.getInt(STATE_COLOR, t.color) ?: t.color
            if (t is TrackEditTarget && savedInstanceState != null) {
                t.stagedVisibility = savedInstanceState.getBoolean(STATE_VISIBLE, t.stagedVisibility)
            }
        }
    }

    private fun setupWindowInsets(view: View) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) return
        ViewCompat.setOnApplyWindowInsetsListener(view) { v, insets ->
            val imeBottom = insets.getInsets(WindowInsetsCompat.Type.ime()).bottom
            val bars = insets.getInsets(
                WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout(),
            )
            v.setPadding(v.paddingLeft, v.paddingTop, v.paddingRight, imeBottom)
            val navBottom = if (imeBottom > 0) 0 else bars.bottom
            nestedScrollView.setPadding(bars.left, 0, bars.right, navBottom)
            toolbar.setPadding(bars.left, bars.top, bars.right, 0)
            WindowInsetsCompat.CONSUMED
        }
    }

    override fun onStart() {
        super.onStart()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            dialog?.window?.let { window ->
                window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING)
                window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS)
                @Suppress("DEPRECATION")
                WindowCompat.setDecorFitsSystemWindows(window, false)
                applyAdaptiveSystemBarIcons()
            }
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            dialog?.window?.let { applyAdaptiveSystemBarIcons() }
        }

        // Focus name and show keyboard for "Unknown Place" bookmarks.
        val unknownPlace = getString(app.organicmaps.sdk.R.string.core_placepage_unknown_place)
        if (target?.isNewlyCreated(unknownPlace) == true) {
            etName.requestFocus()
            etName.selectAll()
            // Recommended way of showing the keyboard on activity start:
            // https://developer.android.com/develop/ui/views/touch-and-input/keyboard-input/visibility#ShowReliably
            WindowCompat.getInsetsController(requireActivity().window, etName)
                .show(WindowInsetsCompat.Type.ime())
        }
    }

    private fun applyAdaptiveSystemBarIcons() {
        val window = dialog?.window ?: return
        val light = !isNightMode()
        WindowCompat.getInsetsController(window, requireView()).apply {
            isAppearanceLightStatusBars = light
            isAppearanceLightNavigationBars = light
        }
    }

    private fun initToolbar() {
        toolbar.setTitle(target?.toolbarTitleRes ?: R.string.placepage_edit_bookmark_button)
        UiUtils.showHomeUpButton(toolbar)
        toolbar.setNavigationOnClickListener { saveAndDismiss() }
        toolbar.inflateMenu(R.menu.menu_edit_bookmark)
        toolbar.menu.setGroupVisible(R.id.group_export, trackTarget != null)
        toolbar.setOnMenuItemClickListener { item ->
            when (item.itemId) {
                R.id.action_delete -> {
                    confirmDelete()
                    true
                }

                R.id.action_export_kmz -> {
                    exportTrack(FileType.Kml)
                    true
                }

                R.id.action_export_gpx -> {
                    exportTrack(FileType.Gpx)
                    true
                }

                R.id.action_export_geojson -> {
                    exportTrack(FileType.GeoJson)
                    true
                }

                else -> false
            }
        }
    }

    private fun exportTrack(fileType: FileType) {
        val t = trackTarget ?: return
        BookmarksSharingHelper.INSTANCE.prepareTrackForSharing(requireActivity(), shareLauncher, t.id, fileType)
    }

    private fun confirmDelete() {
        val t = trackTarget
        if (t == null) {
            deleteAndDismiss()
            return
        }
        DeleteConfirmationDialogFragment.showDialog(
            childFragmentManager,
            getString(R.string.delete_track_dialog_title, t.name),
        )
    }

    private fun toggleVisibility() {
        val t = trackTarget ?: return
        t.stagedVisibility = !t.stagedVisibility
        refreshVisibility()
    }

    private fun refreshVisibility() {
        val t = trackTarget
        ivVisibility.isVisible = t != null
        if (t == null) return
        ivVisibility.setImageResource(if (t.stagedVisibility) R.drawable.ic_show else R.drawable.ic_hide)
        ivVisibility.contentDescription =
            getString(if (t.stagedVisibility) R.string.hide_track else R.string.show_track)
    }

    override fun onCancel(dialog: DialogInterface) {
        // Fires on user-initiated cancel: system back button or swipe-back gesture.
        // Not fired on lifecycle destruction (config change, process kill).
        saveIfNotDeleted()
        super.onCancel(dialog)
    }

    private fun saveAndDismiss() {
        saveIfNotDeleted()
        // Hiding the selected track may already have closed the Place Page hosting this dialog.
        if (isAdded) dismiss()
    }

    private fun saveIfNotDeleted() {
        if (deleted) return
        val t = target ?: return
        val newName = etName.text.toString()
        val newDescription = etDescription.text.toString()
        if (!t.isDirty(newName, newDescription, bookmarkCategory)) return
        val movedFromCategory = t.save(newName, newDescription, bookmarkCategory)
        parentFragmentManager.setFragmentResult(
            REQUEST_KEY,
            bundleOf(
                RESULT_ACTION to ACTION_SAVED,
                RESULT_SAVED_ID to t.id,
                RESULT_MOVED_FROM_CATEGORY to movedFromCategory,
            ),
        )
        trackTarget?.applyStagedVisibility()
    }

    private fun deleteAndDismiss() {
        deleted = true
        target?.delete()
        if (!isAdded) return
        parentFragmentManager.setFragmentResult(
            REQUEST_KEY,
            bundleOf(RESULT_ACTION to ACTION_DELETED),
        )
        dismiss()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putParcelable(STATE_BOOKMARK_CATEGORY, bookmarkCategory)
        target?.let { outState.putInt(STATE_COLOR, it.color) }
        trackTarget?.let { outState.putBoolean(STATE_VISIBLE, it.stagedVisibility) }
    }

    private fun selectBookmarkSet() {
        if (target == null) return
        ChooseBookmarkCategoryFragment().also { fragment ->
            fragment.arguments = bundleOf(
                ChooseBookmarkCategoryFragment.CATEGORY_ID to bookmarkCategory.id,
            )
            fragment.show(childFragmentManager, null)
        }
    }

    private fun selectBookmarkColor() {
        val t = target ?: return
        ColorPickerFragment.show(childFragmentManager, t.color)
    }

    override fun onColorSet(@ColorInt color: Int) {
        val t = target ?: return
        if (t.color == color) return
        t.color = color
        refreshSwatch()
    }

    private fun refreshSwatch() {
        val t = target ?: return
        ivColor.setImageDrawable(t.buildSwatch(requireContext()))
    }

    private fun refreshCategory() {
        tvBookmarkGroup.setText(bookmarkCategory.name)
    }

    private fun bindTarget() {
        val t = target ?: return
        if (etName.text.isNullOrEmpty()) etName.setText(t.name)
        if (etDescription.text.isNullOrEmpty()) etDescription.setText(t.description)
        refreshCategory()
        refreshSwatch()
        refreshVisibility()
    }

    override fun onCategoryChanged(newCategory: BookmarkCategory) {
        bookmarkCategory = newCategory
        refreshCategory()
    }

    private enum class TargetKind { BOOKMARK, TRACK }

    companion object {
        const val REQUEST_KEY = "EditBookmarkFragmentResult"
        const val RESULT_ACTION = "action"
        const val ACTION_SAVED = "saved"
        const val ACTION_DELETED = "deleted"
        const val RESULT_SAVED_ID = "savedId"
        const val RESULT_MOVED_FROM_CATEGORY = "movedFromCategory"

        private const val EXTRA_CATEGORY_ID = "CategoryId"
        private const val EXTRA_ID = "BookmarkTrackId"
        private const val EXTRA_KIND = "BookmarkType"
        private const val STATE_BOOKMARK_CATEGORY = "bookmark_category"
        private const val STATE_COLOR = "color"
        private const val STATE_VISIBLE = "visible"
        private const val TAG = "EditBookmarkFragment"

        @JvmStatic
        fun editBookmark(categoryId: Long, bookmarkId: Long, manager: FragmentManager) =
            show(manager, argsFor(TargetKind.BOOKMARK, categoryId, bookmarkId))

        @JvmStatic
        fun editTrack(categoryId: Long, trackId: Long, manager: FragmentManager) =
            show(manager, argsFor(TargetKind.TRACK, categoryId, trackId))

        private fun argsFor(kind: TargetKind, categoryId: Long, id: Long): Bundle = bundleOf(
            EXTRA_KIND to kind.name,
            EXTRA_CATEGORY_ID to categoryId,
            EXTRA_ID to id,
        )

        private fun show(manager: FragmentManager, args: Bundle) {
            EditBookmarkFragment().apply {
                arguments = args
                show(manager, TAG)
            }
        }
    }
}
