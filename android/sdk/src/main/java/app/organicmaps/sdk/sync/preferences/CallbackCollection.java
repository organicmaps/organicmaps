package app.organicmaps.sdk.sync.preferences;

import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

public class CallbackCollection<T>
{
  private static final String TAG = CallbackCollection.class.getSimpleName();
  private static final int INITIAL_CALLBACK_CAPACITY = 2;
  private final List<T> mList = new ArrayList<>(INITIAL_CALLBACK_CAPACITY);

  void notifyAll(Consumer<T> invoker)
  {
    for (T callback : mList)
    {
      try
      {
        invoker.accept(callback);
      }
      catch (Exception e)
      {
        Logger.e(TAG, "Error invoking callback", e);
      }
    }
  }

  void register(T callback)
  {
    mList.add(callback);
  }

  void unregister(T callback)
  {
    mList.remove(callback);
  }
}
