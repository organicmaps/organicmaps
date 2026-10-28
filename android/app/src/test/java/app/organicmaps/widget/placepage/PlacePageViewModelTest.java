package app.organicmaps.widget.placepage;

import static app.organicmaps.widget.placepage.PlacePageButtons.ButtonType.BACK;
import static app.organicmaps.widget.placepage.PlacePageButtons.ButtonType.TRACK_RECORDING_DELETE;
import static app.organicmaps.widget.placepage.PlacePageButtons.ButtonType.TRACK_RECORDING_SAVE;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;

import androidx.arch.core.executor.ArchTaskExecutor;
import androidx.arch.core.executor.TaskExecutor;
import androidx.lifecycle.Observer;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class PlacePageViewModelTest
{
  @Before
  public void runLiveDataOnTestThread()
  {
    ArchTaskExecutor.getInstance().setDelegate(new TaskExecutor() {
      @Override
      public void executeOnDiskIO(Runnable runnable)
      {
        runnable.run();
      }

      @Override
      public void postToMainThread(Runnable runnable)
      {
        runnable.run();
      }

      @Override
      public boolean isMainThread()
      {
        return true;
      }
    });
  }

  @After
  public void restoreExecutor()
  {
    ArchTaskExecutor.getInstance().setDelegate(null);
  }

  @Test
  public void saveAppearsWithoutReselectingTheRecording()
  {
    final PlacePageViewModel model = new PlacePageViewModel();
    final MapObject recording = MapObject.createMapObject(MapObject.TRACK_RECORDING, "", "", 0, 0);
    model.setMapObject(recording);
    model.setCurrentButtons(List.of(BACK, TRACK_RECORDING_DELETE));
    final AtomicInteger buttonUpdates = new AtomicInteger();
    final AtomicInteger selectionUpdates = new AtomicInteger();
    final Observer<List<PlacePageButtons.ButtonType>> buttonsObserver = buttons -> buttonUpdates.incrementAndGet();
    final Observer<MapObject> selectionObserver = object -> selectionUpdates.incrementAndGet();
    model.getCurrentButtons().observeForever(buttonsObserver);
    model.getMapObject().observeForever(selectionObserver);
    buttonUpdates.set(0);
    selectionUpdates.set(0);
    try
    {
      model.setTrackRecordingSaveVisible(false);
      assertEquals(0, buttonUpdates.get());
      model.setTrackRecordingSaveVisible(true);
      assertEquals(List.of(BACK, TRACK_RECORDING_DELETE, TRACK_RECORDING_SAVE), model.getCurrentButtons().getValue());
      assertEquals(1, buttonUpdates.get());

      final List<PlacePageButtons.ButtonType> buttons = model.getCurrentButtons().getValue();
      model.setTrackRecordingSaveVisible(true);
      assertSame(buttons, model.getCurrentButtons().getValue());
      assertEquals(1, buttonUpdates.get());

      model.setTrackRecordingSaveVisible(false);
      assertEquals(List.of(BACK, TRACK_RECORDING_DELETE), model.getCurrentButtons().getValue());
      assertEquals(2, buttonUpdates.get());
      assertSame(recording, model.getMapObject().getValue());
      assertEquals(0, selectionUpdates.get());
    }
    finally
    {
      model.getCurrentButtons().removeObserver(buttonsObserver);
      model.getMapObject().removeObserver(selectionObserver);
    }
  }
}
