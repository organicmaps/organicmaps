package app.organicmaps.widget.placepage;

import static app.organicmaps.widget.placepage.PlacePageButtons.ButtonType.BACK;
import static app.organicmaps.widget.placepage.PlacePageButtons.ButtonType.BOOKMARK_SAVE;
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

public class RecordingButtonsTest
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
    try
    {
      model.updateTrackRecordingButtons(true);
      assertEquals(1, buttonUpdates.get());
      model.updateTrackRecordingButtons(false);
      assertEquals(List.of(BACK, TRACK_RECORDING_DELETE, TRACK_RECORDING_SAVE), model.getCurrentButtons().getValue());
      assertEquals(2, buttonUpdates.get());

      final List<PlacePageButtons.ButtonType> buttons = model.getCurrentButtons().getValue();
      model.updateTrackRecordingButtons(false);
      assertSame(buttons, model.getCurrentButtons().getValue());
      assertEquals(2, buttonUpdates.get());
      assertSame(recording, model.getMapObject().getValue());
      assertEquals(1, selectionUpdates.get());

      model.updateTrackRecordingButtons(true);
      assertEquals(List.of(BACK, TRACK_RECORDING_DELETE), model.getCurrentButtons().getValue());
      assertEquals(3, buttonUpdates.get());
    }
    finally
    {
      model.getCurrentButtons().removeObserver(buttonsObserver);
      model.getMapObject().removeObserver(selectionObserver);
    }
  }

  @Test
  public void recordingUpdateLeavesOtherSelectionsAndActionsAlone()
  {
    final PlacePageViewModel model = new PlacePageViewModel();
    final List<PlacePageButtons.ButtonType> ordinaryButtons = List.of(BOOKMARK_SAVE);
    model.updateTrackRecordingButtons(false);
    model.setCurrentButtons(ordinaryButtons);
    model.setMapObject(MapObject.createMapObject(MapObject.POI, "", "", 0, 0));
    model.updateTrackRecordingButtons(false);
    assertSame(ordinaryButtons, model.getCurrentButtons().getValue());

    model.setMapObject(MapObject.createMapObject(MapObject.TRACK_RECORDING, "", "", 0, 0));
    model.updateTrackRecordingButtons(false);
    assertSame(ordinaryButtons, model.getCurrentButtons().getValue());

    model.setCurrentButtons(List.of(TRACK_RECORDING_DELETE, TRACK_RECORDING_SAVE));
    model.updateTrackRecordingButtons(false);
    assertEquals(List.of(TRACK_RECORDING_DELETE, TRACK_RECORDING_SAVE), model.getCurrentButtons().getValue());
  }
}
