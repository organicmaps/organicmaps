package app.organicmaps;

import static org.junit.Assert.assertFalse;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.RETURNS_SELF;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import android.content.DialogInterface;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.routing.RoutingController;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class MwmActivityRoutingTest
{
  @Test
  public void myPositionDestinationOffersCompleteReversal()
  {
    checkStartNotice(true, true);
  }

  @Test
  public void ordinaryDestinationUsesLocationAtConfirmation()
  {
    checkStartNotice(false, true);
  }

  @Test
  public void missingLocationOnlyOffersDismissal()
  {
    checkStartNotice(false, false);
  }

  private static void checkStartNotice(boolean destinationIsMyPosition, boolean hasLocation)
  {
    final MwmActivity activity = mock(MwmActivity.class, CALLS_REAL_METHODS);
    final MwmApplication application = mock(MwmApplication.class);
    final LocationHelper locationHelper = mock(LocationHelper.class);
    when(application.getLocationHelper()).thenReturn(locationHelper);
    final MapObject position = mock(MapObject.class);
    when(locationHelper.getMyPosition()).thenReturn(hasLocation ? position : null);

    final RoutingController controller = mock(RoutingController.class);
    when(controller.getStartPoint()).thenReturn(mock(MapObject.class));
    final MapObject destination = mock(MapObject.class);
    when(destination.isMyPosition()).thenReturn(destinationIsMyPosition);
    when(controller.getEndPoint()).thenReturn(destination);

    try (MockedStatic<MwmApplication> app = mockStatic(MwmApplication.class);
         MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedConstruction<MaterialAlertDialogBuilder> dialogs =
             mockConstruction(MaterialAlertDialogBuilder.class, withSettings().defaultAnswer(RETURNS_SELF)))
    {
      app.when(() -> MwmApplication.from(activity)).thenReturn(application);
      routing.when(RoutingController::get).thenReturn(controller);

      assertFalse(activity.showStartPointNotice());
      final MaterialAlertDialogBuilder dialog = dialogs.constructed().get(0);
      if (!hasLocation)
      {
        verify(dialog).setMessage(R.string.unknown_current_position);
        verify(dialog).setPositiveButton(R.string.ok, null);
      }
      else
      {
        final ArgumentCaptor<DialogInterface.OnClickListener> action =
            ArgumentCaptor.forClass(DialogInterface.OnClickListener.class);
        verify(dialog).setPositiveButton(eq(destinationIsMyPosition ? R.string.reverse_route : R.string.ok),
                                         action.capture());
        final MapObject currentPosition = mock(MapObject.class);
        when(locationHelper.getMyPosition()).thenReturn(currentPosition);
        action.getValue().onClick(null, DialogInterface.BUTTON_POSITIVE);
        if (destinationIsMyPosition)
          verify(controller).reverseRoute();
        else
          verify(controller).setStartPoint(currentPosition);
      }
      verify(controller, never()).start();
    }
  }
}
