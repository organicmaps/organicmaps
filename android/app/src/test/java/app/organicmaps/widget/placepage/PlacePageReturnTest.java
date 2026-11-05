package app.organicmaps.widget.placepage;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verifyNoInteractions;

import androidx.fragment.app.FragmentActivity;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.util.Utils;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.MockedStatic;

public class PlacePageReturnTest
{
  @Test
  public void routeReturnDoesNotFinishTheActivityOrSendAPointPickerResult() throws ReflectiveOperationException
  {
    final PlacePageController controller = mock(PlacePageController.class, CALLS_REAL_METHODS);
    final FragmentActivity activity = mock(FragmentActivity.class);
    doReturn(activity).when(controller).requireActivity();
    doReturn(false).when(controller).shouldReturnApiResult();
    final Field mapObject = PlacePageController.class.getDeclaredField("mMapObject");
    mapObject.setAccessible(true);
    mapObject.set(controller, MapObject.createMapObject(MapObject.POI, "Stop", "", 1, 2));

    try (MockedStatic<Utils> utils = mockStatic(Utils.class))
    {
      controller.onPlacePageButtonClick(PlacePageButtons.ButtonType.BACK);

      utils.verify(() -> Utils.returnToCaller(activity));
      verifyNoInteractions(activity);
    }
  }
}
