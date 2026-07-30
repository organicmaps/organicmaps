package app.organicmaps.bookmarks

import android.content.Intent
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.view.WindowManager
import androidx.activity.SystemBarStyle
import androidx.activity.result.ActivityResultLauncher
import androidx.fragment.app.Fragment
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragmentActivity
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory

class BookmarkCategorySettingsActivity : BaseMwmFragmentActivity() {

    override fun getStatusBarStyle(): SystemBarStyle = SystemBarStyle.auto(Color.TRANSPARENT, Color.TRANSPARENT)

    /**
     * The manifest asks for adjustResize, which below API 30 is what makes WindowInsetsCompat report
     * Type.ime() at all. From API 30 the platform honours it and resizes the window, so the adjust
     * bits are swapped for ADJUST_NOTHING there and [BookmarkCategorySettingsFragment] stays the
     * only thing compensating for the keyboard. The state bits the manifest declares are kept.
     */
    override fun onSafeCreate(savedInstanceState: Bundle?) {
        super.onSafeCreate(savedInstanceState)
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) return
        val state = window.attributes.softInputMode and WindowManager.LayoutParams.SOFT_INPUT_MASK_ADJUST.inv()
        window.setSoftInputMode(state or WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING)
    }

    override fun getContentLayoutResId(): Int = R.layout.fragment_container_layout

    override fun getFragmentContentResId(): Int = R.id.fragment_container

    override fun getFragmentClass(): Class<out Fragment> = BookmarkCategorySettingsFragment::class.java

    companion object {
        const val EXTRA_BOOKMARK_CATEGORY = "bookmark_category"

        @JvmStatic
        fun startForResult(
            fragment: Fragment,
            startBookmarkSettingsForResult: ActivityResultLauncher<Intent>,
            category: BookmarkCategory,
        ) {
            val intent = Intent(fragment.requireActivity(), BookmarkCategorySettingsActivity::class.java)
                .putExtra(EXTRA_BOOKMARK_CATEGORY, category)
            startBookmarkSettingsForResult.launch(intent)
        }
    }
}
