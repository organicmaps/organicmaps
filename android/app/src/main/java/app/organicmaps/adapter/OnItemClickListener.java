package app.organicmaps.adapter;

import android.view.View;
import androidx.annotation.NonNull;

public interface OnItemClickListener<T>
{
  void onItemClick(@NonNull View v, @NonNull T item);
}
