package app.organicmaps.bookmarks

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.bookmarks.Holders.CategoryViewHolder
import app.organicmaps.bookmarks.Holders.GeneralViewHolder
import app.organicmaps.bookmarks.Holders.HeaderViewHolder
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory
import app.organicmaps.sdk.bookmarks.data.BookmarkManager

class BookmarkCategoriesAdapter(
    categories: List<BookmarkCategory>,
    private val onCategoryClick: (BookmarkCategory) -> Unit,
    private val onCategoryMenuClick: (BookmarkCategory) -> Unit,
    private val categoryListCallback: CategoryListCallback,
) : RecyclerView.Adapter<RecyclerView.ViewHolder>() {

    var bookmarkCategories: List<BookmarkCategory> = categories
        private set

    private enum class RowType { HEADER, CATEGORY, ACTION }

    /** The rows of the card below the lists, in the order they are shown. */
    private enum class Action(
        @param:DrawableRes val icon: Int,
        @param:StringRes val title: Int,
        val click: (CategoryListCallback) -> Unit,
    ) {
        ADD(R.drawable.ic_add_list, R.string.bookmarks_create_new_group, CategoryListCallback::onAddButtonClick),
        IMPORT(R.drawable.ic_import, R.string.bookmarks_import, CategoryListCallback::onImportButtonClick),
        EXPORT(R.drawable.ic_export, R.string.bookmarks_export, CategoryListCallback::onExportButtonClick),
    }

    // Every change goes through an edit session in the core, which notifies the listener that calls setItems().
    private val massOperationAction = object : HeaderViewHolder.HeaderAction {
        override fun onHideAll() = BookmarkManager.INSTANCE.setAllCategoriesVisibility(false)

        override fun onShowAll() = BookmarkManager.INSTANCE.setAllCategoriesVisibility(true)
    }

    private val categoryCount: Int
        get() = bookmarkCategories.size

    fun setItems(categories: List<BookmarkCategory>) {
        bookmarkCategories = categories
        notifyDataSetChanged()
    }

    private fun rowTypeAt(position: Int): RowType = when {
        position < HEADER_COUNT -> RowType.HEADER
        position < HEADER_COUNT + categoryCount -> RowType.CATEGORY
        else -> RowType.ACTION
    }

    override fun getItemCount(): Int = if (categoryCount == 0) 0 else HEADER_COUNT + categoryCount + Action.entries.size

    override fun getItemViewType(position: Int): Int = rowTypeAt(position).ordinal

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        return when (RowType.entries[viewType]) {
            RowType.HEADER ->
                HeaderViewHolder(inflater.inflate(R.layout.item_bookmark_group_list_header, parent, false))

            RowType.CATEGORY -> {
                val view = inflater.inflate(R.layout.item_bookmark_category, parent, false)
                CategoryViewHolder(view).also { holder ->
                    view.setOnClickListener { onCategoryClick(holder.entity) }
                    view.setOnLongClickListener {
                        onCategoryMenuClick(holder.entity)
                        true
                    }
                    holder.setVisibilityListener { holder.entity.toggleVisibility() }
                    holder.setMoreButtonClickListener { onCategoryMenuClick(holder.entity) }
                }
            }

            RowType.ACTION -> GeneralViewHolder(inflater.inflate(R.layout.item_bookmark_button, parent, false))
        }
    }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        val index = position - HEADER_COUNT
        when (rowTypeAt(position)) {
            RowType.HEADER -> (holder as HeaderViewHolder).bindHeader()
            RowType.CATEGORY -> (holder as CategoryViewHolder).bindCategory(index)
            RowType.ACTION -> (holder as GeneralViewHolder).bindAction(index - categoryCount)
        }
    }

    private fun HeaderViewHolder.bindHeader() {
        setAction(massOperationAction, BookmarkManager.INSTANCE.areAllCategoriesInvisible())
        text.setText(R.string.bookmark_lists)
    }

    private fun CategoryViewHolder.bindCategory(index: Int) {
        val category = bookmarkCategories[index]
        entity = category
        setName(category.name)
        setSize()
        setVisibilityState(category.isVisible)
        bindCardPosition(index == 0, index == categoryCount - 1)
    }

    private fun GeneralViewHolder.bindAction(index: Int) {
        val action = Action.entries[index]
        image.setImageResource(action.icon)
        text.setText(action.title)
        itemView.setOnClickListener { action.click(categoryListCallback) }
        bindCardPosition(index == 0, index == Action.entries.lastIndex)
        // Action rows share one view type, so the gap above the first one is reset on rebind.
        (itemView.layoutParams as RecyclerView.LayoutParams).topMargin =
            if (index == 0) itemView.resources.getDimensionPixelSize(R.dimen.margin_base) else 0
    }

    private companion object {
        const val HEADER_COUNT = 1
    }
}
