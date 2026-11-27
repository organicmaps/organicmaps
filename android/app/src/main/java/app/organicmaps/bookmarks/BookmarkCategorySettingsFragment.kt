package app.organicmaps.bookmarks

import android.os.Bundle
import android.text.InputFilter
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.annotation.ColorInt
import androidx.annotation.StringRes
import androidx.core.view.MenuProvider
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.isVisible
import androidx.core.view.updatePadding
import app.organicmaps.R
import app.organicmaps.base.BaseMwmToolbarFragment
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
import app.organicmaps.sdk.bookmarks.data.DataChangedListener
import app.organicmaps.util.Utils
import app.organicmaps.widget.ToolbarController
import app.organicmaps.widget.colorpicker.ColorPickerFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.textfield.TextInputEditText

class BookmarkCategorySettingsFragment :
    BaseMwmToolbarFragment(),
    ColorPickerFragment.OnColorChangeListener {

    private lateinit var category: BookmarkCategory
    private lateinit var editCategoryNameView: TextInputEditText
    private lateinit var editDescView: TextInputEditText
    private lateinit var colorBookmarksBtn: View
    private lateinit var colorTracksBtn: View

    // Persisted: the picker is a child fragment that outlives a rotation or process recreation and
    // pushes its result blindly into the parent, so a stale target would recolor the wrong thing.
    private var colorTarget = ColorTarget.BOOKMARKS

    private val categoriesListener = DataChangedListener { updateColorButtonsVisibility() }

    private val menuProvider = object : MenuProvider {
        override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) {
            menuInflater.inflate(R.menu.menu_done, menu)
        }

        override fun onMenuItemSelected(menuItem: MenuItem): Boolean {
            if (menuItem.itemId != R.id.done) {
                return false
            }
            onEditDoneClicked()
            return true
        }
    }

    private val editableCategoryName: String
        get() = editCategoryNameView.editableText.toString().trim()

    private val editableCategoryDesc: String
        get() = editDescView.editableText.toString().trim()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        category = requireNotNull(
            Utils.getParcelable(
                requireArguments(),
                BookmarkCategorySettingsActivity.EXTRA_BOOKMARK_CATEGORY,
                BookmarkCategory::class.java,
            ),
        )
        val savedTarget = savedInstanceState?.getString(STATE_COLOR_TARGET)
        colorTarget = ColorTarget.entries.firstOrNull { it.name == savedTarget } ?: ColorTarget.BOOKMARKS
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putString(STATE_COLOR_TARGET, colorTarget.name)
    }

    override fun onStart() {
        super.onStart()
        BookmarkManager.INSTANCE.addCategoriesUpdatesListener(categoriesListener)
    }

    override fun onStop() {
        super.onStop()
        BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(categoriesListener)
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
        inflater.inflate(R.layout.fragment_bookmark_category_settings, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        requireActivity().addMenuProvider(menuProvider, viewLifecycleOwner)
        initViews(view)
        setupWindowInsets(view)
    }

    /**
     * A length filter truncates whatever setText() is given, and both initViews() and the view state
     * restore that precedes this callback go through setText(). Installing the filter only here caps
     * typing without rewriting a stored name — imported ones are not capped, see
     * BookmarkManager::CreateCategories.
     */
    override fun onViewStateRestored(savedInstanceState: Bundle?) {
        super.onViewStateRestored(savedInstanceState)
        editCategoryNameView.filters = arrayOf(InputFilter.LengthFilter(CategoryValidator.MAX_NAME_LENGTH))
    }

    /**
     * The window is never resized for the IME (see [BookmarkCategorySettingsActivity]), so padding
     * the root is what shrinks the scrollable content and leaves the toolbar above it in place.
     * Deliberately not consumed: the toolbar has its own listener from [ToolbarController].
     */
    private fun setupWindowInsets(root: View) {
        val scroll = root.findViewById<View>(R.id.category_settings_scroll)
        ViewCompat.setOnApplyWindowInsetsListener(root) { v, windowInsets ->
            val imeBottom = windowInsets.getInsets(WindowInsetsCompat.Type.ime()).bottom
            val bars = windowInsets.getInsets(
                WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout(),
            )
            v.updatePadding(bottom = imeBottom)
            val navBottom = if (imeBottom > 0) 0 else bars.bottom
            scroll.updatePadding(left = bars.left, right = bars.right, bottom = navBottom)
            windowInsets
        }
    }

    private fun initViews(root: View) {
        editCategoryNameView = root.findViewById(R.id.edit_list_name_view)
        editCategoryNameView.setText(category.name)
        editCategoryNameView.requestFocus()

        editDescView = root.findViewById(R.id.edit_description)
        editDescView.setText(category.description)

        colorBookmarksBtn = root.findViewById(R.id.color_bookmarks_btn)
        colorBookmarksBtn.setOnClickListener { showBookmarkColorPicker() }
        colorTracksBtn = root.findViewById(R.id.color_tracks_btn)
        colorTracksBtn.setOnClickListener { showTrackColorPicker() }

        updateColorButtonsVisibility()
    }

    private fun updateColorButtonsVisibility() {
        val current = BookmarkManager.INSTANCE.getCategoryById(category.id)
        colorBookmarksBtn.isVisible = current.bookmarksCount > 0
        colorTracksBtn.isVisible = current.tracksCount > 0
    }

    private fun onEditDoneClicked() {
        val newCategoryName = editableCategoryName
        if (!validateCategoryName(newCategoryName)) {
            return
        }

        if (newCategoryName != category.name) {
            category.name = newCategoryName
        }

        val newDescription = editableCategoryDesc
        if (newDescription != category.description) {
            category.description = newDescription
        }

        requireActivity().finish()
    }

    private fun validateCategoryName(name: String): Boolean {
        if (name.isEmpty()) {
            showNameError(
                R.string.bookmarks_error_title_empty_list_name,
                R.string.bookmarks_error_message_empty_list_name,
            )
            return false
        }

        if (BookmarkManager.INSTANCE.isUsedCategoryName(name) && name != category.name) {
            showNameError(
                R.string.bookmarks_error_title_list_name_already_taken,
                R.string.bookmarks_error_message_list_name_already_taken,
            )
            return false
        }
        return true
    }

    private fun showNameError(@StringRes titleRes: Int, @StringRes messageRes: Int) {
        MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setTitle(titleRes)
            .setMessage(messageRes)
            .setPositiveButton(R.string.ok, null)
            .show()
    }

    private fun showBookmarkColorPicker() {
        colorTarget = ColorTarget.BOOKMARKS
        ColorPickerFragment.show(childFragmentManager, BookmarkManager.INSTANCE.lastEditedColor)
    }

    private fun showTrackColorPicker() {
        colorTarget = ColorTarget.TRACKS
        ColorPickerFragment().show(childFragmentManager, null)
    }

    override fun onColorSet(@ColorInt color: Int) {
        when (colorTarget) {
            ColorTarget.BOOKMARKS -> category.setCategoryBookmarksColor(color)
            ColorTarget.TRACKS -> category.setCategoryTracksCustomColor(color)
        }
        Toast.makeText(requireContext(), colorTarget.toastRes, Toast.LENGTH_SHORT).show()
    }

    private enum class ColorTarget(@get:StringRes val toastRes: Int) {
        BOOKMARKS(R.string.toast_bookmarks_color_changed),
        TRACKS(R.string.toast_tracks_color_changed),
    }

    companion object {
        private const val STATE_COLOR_TARGET = "color_target"
    }
}
