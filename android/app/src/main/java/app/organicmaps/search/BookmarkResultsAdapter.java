package app.organicmaps.search;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.core.text.HtmlCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.LongConsumer;

/** Ordinary bookmark results, independent of the bookmark's import source. */
final class BookmarkResultsAdapter extends RecyclerView.Adapter<BookmarkResultsAdapter.Holder>
{
  private final List<Long> results = new ArrayList<>();
  private final Map<Long, BookmarkInfo> details = new HashMap<>();
  private final LongConsumer select;
  private boolean expanded;

  BookmarkResultsAdapter(LongConsumer select)
  {
    this.select = select;
  }

  void update(long[] ids)
  {
    if (ids == null)
      expanded = false;
    results.clear();
    details.clear();
    if (ids != null)
      for (long id : ids)
      {
        results.add(id);
      }
    notifyDataSetChanged();
  }

  @Override
  public int getItemCount()
  {
    return SearchResultsPresentation.bookmarkRows(results.size(), expanded);
  }

  @Override
  public int getItemViewType(int position)
  {
    return SearchResultsPresentation.bookmarkRow(position, results.size(), expanded).ordinal();
  }
  @NonNull
  @Override
  public Holder onCreateViewHolder(@NonNull ViewGroup parent, int type)
  {
    return new Holder(LayoutInflater.from(parent.getContext())
                          .inflate(type == SearchResultsPresentation.BookmarkRow.RESULT.ordinal()
                                       ? R.layout.item_search_result
                                       : R.layout.item_search_suggest,
                                   parent, false));
  }
  @Override
  public void onBindViewHolder(@NonNull Holder holder, int position)
  {
    holder.itemView.setVisibility(View.VISIBLE);
    if (getItemViewType(position) != SearchResultsPresentation.BookmarkRow.RESULT.ordinal())
    {
      TextView label = (TextView) holder.itemView;
      if (getItemViewType(position) == SearchResultsPresentation.BookmarkRow.LIMIT.ordinal())
      {
        label.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, 0, 0);
        label.setText(holder.itemView.getContext().getString(R.string.bookmark_search_limit_reached,
                                                             SearchResultsPresentation.BOOKMARK_LIMIT));
        label.setTextColor(
            app.organicmaps.util.ThemeUtils.getColor(label.getContext(), android.R.attr.textColorSecondary));
        label.setOnClickListener(null);
      }
      else
      {
        label.setCompoundDrawablesRelativeWithIntrinsicBounds(
            expanded ? R.drawable.ic_expand_less : R.drawable.ic_expand_more, 0, 0, 0);
        app.organicmaps.util.Graphics.tint(label, androidx.appcompat.R.attr.colorAccent);
        label.setText(expanded
                          ? holder.itemView.getContext().getString(R.string.bookmark_search_show_less)
                          : holder.itemView.getContext().getString(R.string.bookmark_search_show_all, results.size()));
        label.setOnClickListener(view -> {
          expanded = !expanded;
          notifyDataSetChanged();
        });
      }
      return;
    }
    BookmarkInfo info = details.computeIfAbsent(results.get(position), BookmarkManager.INSTANCE::getBookmarkInfo);
    if (info == null)
    {
      // The bookmark can be deleted between the native result batch and binding. A new search
      // is scheduled by the bookmarks-change listener; do not leave a stale row clickable.
      holder.itemView.setOnClickListener(null);
      holder.itemView.setVisibility(View.INVISIBLE);
      return;
    }
    ((TextView) holder.itemView.findViewById(R.id.title)).setText(info.getName());
    String description = holder.itemView.getContext().getString(R.string.bookmarks);
    BookmarkCategory category = BookmarkManager.INSTANCE.getCategoryById(info.getCategoryId());
    if (category != null && !category.getName().isEmpty())
      description += " • " + category.getName();
    ((TextView) holder.itemView.findViewById(R.id.description)).setText(description);
    ((TextView) holder.itemView.findViewById(R.id.region))
        .setText(HtmlCompat.fromHtml(info.getDescription(), HtmlCompat.FROM_HTML_MODE_LEGACY));
    holder.itemView.findViewById(R.id.open).setVisibility(View.GONE);
    holder.itemView.findViewById(R.id.distance).setVisibility(View.GONE);
    holder.itemView.setOnClickListener(view -> select.accept(info.getBookmarkId()));
  }
  static final class Holder extends RecyclerView.ViewHolder
  {
    Holder(View view)
    {
      super(view);
    }
  }
}
