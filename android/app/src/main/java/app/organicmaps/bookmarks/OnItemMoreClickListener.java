package app.organicmaps.bookmarks;

import android.view.View;
import androidx.annotation.NonNull;

public interface OnItemMoreClickListener<T>
{
  void onItemMoreClick(@NonNull View v, @NonNull T item);
}
