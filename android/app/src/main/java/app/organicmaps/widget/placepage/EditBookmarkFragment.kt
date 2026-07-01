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
import androidx.annotation.ColorInt
import androidx.appcompat.widget.Toolbar
import androidx.core.os.BundleCompat
import androidx.core.os.bundleOf
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.widget.NestedScrollView
import androidx.fragment.app.FragmentManager
import app.organicmaps.R
import app.organicmaps.base.BaseMwmDialogFragment
import app.organicmaps.bookmarks.ChooseBookmarkCategoryFragment
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
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
    private lateinit var nestedScrollView: NestedScrollView
    private lateinit var toolbar: Toolbar
    private lateinit var bookmarkCategory: BookmarkCategory

    private var target: EditTarget? = null
    private var deleted: Boolean = false

    override fun getCustomTheme(): Int = fullscreenTheme

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
        inflater.inflate(R.layout.fragment_edit_bookmark, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val args = requireArguments()

        etName = view.findViewById(R.id.et__bookmark_name)
        etDescription = view.findViewById(R.id.et__description)
        tvBookmarkGroup = view.findViewById<TextInputEditText>(R.id.tv__bookmark_set)
            .apply { setOnClickListener { selectBookmarkSet() } }
        ivColor = view.findViewById(R.id.iv__bookmark_color)
        toolbar = view.findViewById(R.id.toolbar)
        nestedScrollView = view.findViewById(R.id.edit_bookmark_scroll)
        view.findViewById<View>(R.id.color_row).setOnClickListener { selectBookmarkColor() }

        target = loadTarget(args, savedInstanceState)
        bookmarkCategory = savedInstanceState?.let {
            BundleCompat.getParcelable(it, STATE_BOOKMARK_CATEGORY, BookmarkCategory::class.java)
        } ?: BookmarkManager.INSTANCE.getCategoryById(args.getLong(EXTRA_CATEGORY_ID))

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
        }
    }

    private fun setupWindowInsets(view: View) {
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

        dialog?.window?.let { window ->
            window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS)
            }
            @Suppress("DEPRECATION")
            WindowCompat.setDecorFitsSystemWindows(window, false)

            val isNight = resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK ==
                Configuration.UI_MODE_NIGHT_YES
            WindowCompat.getInsetsController(window, requireView()).apply {
                isAppearanceLightStatusBars = !isNight
                isAppearanceLightNavigationBars = !isNight
            }
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

    private fun initToolbar() {
        toolbar.setTitle(target?.toolbarTitleRes ?: R.string.placepage_edit_bookmark_button)
        UiUtils.showHomeUpButton(toolbar)
        toolbar.setNavigationOnClickListener { saveAndDismiss() }
        toolbar.inflateMenu(R.menu.menu_edit_bookmark)
        toolbar.setOnMenuItemClickListener { item ->
            when (item.itemId) {
                R.id.action_delete -> {
                    deleteAndDismiss()
                    true
                }

                else -> false
            }
        }
    }

    override fun onCancel(dialog: DialogInterface) {
        // Fires on user-initiated cancel: system back button or swipe-back gesture.
        // Not fired on lifecycle destruction (config change, process kill).
        saveIfNotDeleted()
        super.onCancel(dialog)
    }

    private fun saveAndDismiss() {
        saveIfNotDeleted()
        dismiss()
    }

    private fun saveIfNotDeleted() {
        if (deleted) return
        val t = target ?: return
        val movedFromCategory = t.save(
            newName = etName.text.toString(),
            newDescription = etDescription.text.toString(),
            category = bookmarkCategory,
        )
        parentFragmentManager.setFragmentResult(
            REQUEST_KEY,
            bundleOf(
                RESULT_SAVED_ID to t.id,
                RESULT_MOVED_FROM_CATEGORY to movedFromCategory,
            ),
        )
    }

    private fun deleteAndDismiss() {
        deleted = true
        target?.delete()
        parentFragmentManager.setFragmentResult(REQUEST_KEY, bundleOf(RESULT_DELETED to true))
        dismiss()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putParcelable(STATE_BOOKMARK_CATEGORY, bookmarkCategory)
        target?.let { outState.putInt(STATE_COLOR, it.color) }
    }

    private fun selectBookmarkSet() {
        if (target == null) return
        ChooseBookmarkCategoryFragment().also { fragment ->
            fragment.arguments = bundleOf(
                ChooseBookmarkCategoryFragment.CATEGORY_POSITION to
                    BookmarkManager.INSTANCE.categories.indexOf(bookmarkCategory),
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
    }

    override fun onCategoryChanged(newCategory: BookmarkCategory) {
        bookmarkCategory = newCategory
        refreshCategory()
    }

    private enum class TargetKind { BOOKMARK, TRACK }

    companion object {
        const val REQUEST_KEY = "EditBookmarkFragmentResult"
        const val RESULT_SAVED_ID = "savedId"
        const val RESULT_MOVED_FROM_CATEGORY = "movedFromCategory"
        const val RESULT_DELETED = "deleted"

        private const val EXTRA_CATEGORY_ID = "CategoryId"
        private const val EXTRA_ID = "BookmarkTrackId"
        private const val EXTRA_KIND = "BookmarkType"
        private const val STATE_BOOKMARK_CATEGORY = "bookmark_category"
        private const val STATE_COLOR = "color"
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
