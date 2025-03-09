package app.organicmaps.bookmarks;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.bookmarks.data.BookmarkCategory;
import app.organicmaps.adapter.OnItemClickListener;

import java.util.List;

public class BookmarkWidgetCategoriesAdapter extends RecyclerView.Adapter<BookmarkWidgetCategoriesAdapter.ViewHolder>
{
  private final Context context;
  private final List<BookmarkCategory> categories;
  private OnItemClickListener<BookmarkCategory> clickListener;

  public BookmarkWidgetCategoriesAdapter(Context context, List<BookmarkCategory> categories)
  {
    this.context = context;
    this.categories = categories;
  }

  public void setOnClickListener(OnItemClickListener<BookmarkCategory> listener)
  {
    this.clickListener = listener;
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(context).inflate(R.layout.item_widget_list, parent, false);
    return new ViewHolder(view);
  }

  @Override
  public void onBindViewHolder(@NonNull ViewHolder holder, int position)
  {
    BookmarkCategory category = categories.get(position);
    holder.categoryName.setText(category.getName());
    holder.bookmarkCount.setText(String.valueOf(category.getBookmarksCount()));
    String bookmarkText = context.getResources().getQuantityString(R.plurals.bookmarks_places, category.getBookmarksCount(), category.getBookmarksCount());
    holder.bookmarkCount.setText(bookmarkText);
    holder.itemView.setOnClickListener(v -> {
      if (clickListener != null)
      {
        clickListener.onItemClick(v, category);
      }
    });
  }

  @Override
  public int getItemCount()
  {
    return categories.size();
  }

  public static class ViewHolder extends RecyclerView.ViewHolder
  {
    TextView categoryName;
    TextView bookmarkCount;

    public ViewHolder(View itemView)
    {
      super(itemView);
      categoryName = itemView.findViewById(R.id.name);
      bookmarkCount = itemView.findViewById(R.id.size);
    }
  }
}
