package app.organicmaps.bookmarks;

import android.view.View;
import androidx.annotation.NonNull;

public interface OnItemLongClickListener<T>
{
  void onItemLongClick(@NonNull View v, @NonNull T item);
}
