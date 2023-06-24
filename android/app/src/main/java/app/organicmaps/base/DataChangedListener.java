package app.organicmaps.base;

public interface DataChangedListener<T> extends Detachable<T>
{
  void onChanged();
}
