package app.organicmaps.bookmarks

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import androidx.core.view.updateLayoutParams
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.adapter.OnItemClickListener
import app.organicmaps.bookmarks.Holders.CategoryViewHolder
import app.organicmaps.bookmarks.Holders.GeneralViewHolder
import app.organicmaps.bookmarks.Holders.HeaderViewHolder
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory
import app.organicmaps.sdk.bookmarks.data.BookmarkManager

class BookmarkCategoriesAdapter(context: Context, categories: List<BookmarkCategory>) :
    BaseBookmarkCategoryAdapter<RecyclerView.ViewHolder>(context.applicationContext, categories) {

    var onClickListener: OnItemClickListener<BookmarkCategory>? = null
    var onMoreClickListener: OnItemMoreClickListener<BookmarkCategory>? = null
    var onLongClickListener: OnItemLongClickListener<BookmarkCategory>? = null
    var categoryListCallback: CategoryListCallback? = null

    /** The header, then every category, then the actions. A row carries the index it holds within its kind. */
    private sealed interface Row {
        data object Header : Row

        data class Category(val index: Int) : Row

        data class ActionRow(val index: Int) : Row
    }

    /** RecyclerView identifies a holder by an Int, so every Row maps to one of these. */
    private enum class RowType { HEADER, CATEGORY, ACTION }

    /** The rows of the card below the lists, in the order they are shown. Adding one here is enough. */
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

    private fun rowAt(position: Int): Row {
        val index = position - HEADER_COUNT
        return when {
            position == HEADER_POSITION -> Row.Header
            index < categoryCount -> Row.Category(index)
            else -> Row.ActionRow(index - categoryCount)
        }
    }

    override fun getItemCount(): Int = if (categoryCount == 0) 0 else HEADER_COUNT + categoryCount + ACTIONS.size

    override fun getItemViewType(position: Int): Int = when (rowAt(position)) {
        Row.Header -> RowType.HEADER
        is Row.Category -> RowType.CATEGORY
        is Row.ActionRow -> RowType.ACTION
    }.ordinal

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        return when (RowType.entries[viewType]) {
            RowType.HEADER ->
                HeaderViewHolder(inflater.inflate(R.layout.item_bookmark_group_list_header, parent, false))

            RowType.CATEGORY -> {
                val view = inflater.inflate(R.layout.item_bookmark_category, parent, false)
                CategoryViewHolder(view).also { holder ->
                    view.setOnClickListener { onClickListener?.onItemClick(it, holder.entity) }
                    view.setOnLongClickListener {
                        onLongClickListener?.onItemLongClick(it, holder.entity)
                        true
                    }
                }
            }

            RowType.ACTION -> GeneralViewHolder(inflater.inflate(R.layout.item_bookmark_button, parent, false))
        }
    }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        when (val row = rowAt(position)) {
            Row.Header -> (holder as HeaderViewHolder).bindHeader()
            is Row.Category -> (holder as CategoryViewHolder).bindCategory(row.index)
            is Row.ActionRow -> (holder as GeneralViewHolder).bindAction(row.index)
        }
    }

    private fun HeaderViewHolder.bindHeader() {
        setAction(massOperationAction, BookmarkManager.INSTANCE.areAllCategoriesInvisible())
        text.setText(R.string.bookmark_lists)
        setSkipDivider(true)
    }

    private fun CategoryViewHolder.bindCategory(index: Int) {
        val category = getCategoryByPosition(index)
        entity = category
        setName(category.name)
        setSize()
        setVisibilityState(category.isVisible)
        setVisibilityListener { entity.toggleVisibility() }
        setMoreButtonClickListener { onMoreClickListener?.onItemMoreClick(it, entity) }
        bindCardPosition(index == 0, index == categoryCount - 1)
    }

    private fun GeneralViewHolder.bindAction(index: Int) {
        val action = ACTIONS[index]
        image.setImageResource(action.icon)
        text.setText(action.title)
        itemView.setOnClickListener { categoryListCallback?.let(action.click) }
        bindCardPosition(index == 0, index == ACTIONS.lastIndex)
        // Only the first action starts a card, and the holders of the other actions are recycled into it.
        itemView.applySectionGap(startsCard = index == 0)
    }

    /** Separates a card from the one above it. */
    private fun View.applySectionGap(startsCard: Boolean) {
        val gap = if (startsCard) resources.getDimensionPixelSize(R.dimen.margin_base) else 0
        if ((layoutParams as RecyclerView.LayoutParams).topMargin != gap) {
            updateLayoutParams<RecyclerView.LayoutParams> { topMargin = gap }
        }
    }

    private companion object {
        const val HEADER_POSITION = 0
        const val HEADER_COUNT = 1

        val ACTIONS = Action.entries
    }
}
