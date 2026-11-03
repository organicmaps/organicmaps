package app.organicmaps;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
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
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class MwmActivityStartPointNoticeTest
{
  private final MwmActivity mActivity = mock(MwmActivity.class, CALLS_REAL_METHODS);
  private final LocationHelper mLocationHelper = mock(LocationHelper.class);
  private final RoutingController mController = mock(RoutingController.class);
  private final MapObject mStartPoint = mock(MapObject.class);
  private final MapObject mEndPoint = mock(MapObject.class);
  private MockedStatic<MwmApplication> mApplicationStatic;
  private MockedStatic<RoutingController> mControllerStatic;
  private MockedConstruction<MaterialAlertDialogBuilder> mDialogs;

  @Before
  public void setUp()
  {
    final MwmApplication application = mock(MwmApplication.class);
    when(application.getLocationHelper()).thenReturn(mLocationHelper);
    when(mController.getStartPoint()).thenReturn(mStartPoint);
    when(mController.getEndPoint()).thenReturn(mEndPoint);

    mApplicationStatic = mockStatic(MwmApplication.class);
    mApplicationStatic.when(() -> MwmApplication.from(mActivity)).thenReturn(application);
    mControllerStatic = mockStatic(RoutingController.class);
    mControllerStatic.when(RoutingController::get).thenReturn(mController);
    mDialogs = mockConstruction(MaterialAlertDialogBuilder.class, withSettings().defaultAnswer(RETURNS_SELF));
  }

  @After
  public void tearDown()
  {
    mDialogs.close();
    mControllerStatic.close();
    mApplicationStatic.close();
  }

  @Test
  public void routeFromMyPositionStartsWithoutNotice()
  {
    when(mStartPoint.isMyPosition()).thenReturn(true);

    assertTrue(mActivity.showStartPointNotice());
    assertTrue(mDialogs.constructed().isEmpty());
  }

  @Test
  public void routeEndingAtMyPositionIsReversed()
  {
    when(mEndPoint.isMyPosition()).thenReturn(true);
    when(mLocationHelper.getMyPosition()).thenReturn(mock(MapObject.class));

    assertFalse(mActivity.showStartPointNotice());
    final MaterialAlertDialogBuilder dialog = mDialogs.constructed().get(0);
    verify(dialog).setMessage(R.string.p2p_reverse_from_current);
    confirm(dialog);

    verify(mController).reverseRoute();
    verify(mController, never()).setStartPoint(any());
  }

  @Test
  public void routeStartsFromPositionAtConfirmation()
  {
    when(mLocationHelper.getMyPosition()).thenReturn(mock(MapObject.class));

    assertFalse(mActivity.showStartPointNotice());
    final MaterialAlertDialogBuilder dialog = mDialogs.constructed().get(0);
    verify(dialog).setMessage(R.string.p2p_reroute_from_current);
    final MapObject confirmedPosition = mock(MapObject.class);
    when(mLocationHelper.getMyPosition()).thenReturn(confirmedPosition);
    confirm(dialog);

    verify(mController).setStartPoint(confirmedPosition);
  }

  @Test
  public void positionLostBeforeConfirmationKeepsRoute()
  {
    when(mLocationHelper.getMyPosition()).thenReturn(mock(MapObject.class));

    assertFalse(mActivity.showStartPointNotice());
    final MaterialAlertDialogBuilder dialog = mDialogs.constructed().get(0);
    when(mLocationHelper.getMyPosition()).thenReturn(null);
    confirm(dialog);

    verify(mController, never()).setStartPoint(any());
  }

  @Test
  public void unknownPositionOnlyInforms()
  {
    when(mEndPoint.isMyPosition()).thenReturn(true);

    assertFalse(mActivity.showStartPointNotice());
    final MaterialAlertDialogBuilder dialog = mDialogs.constructed().get(0);
    verify(dialog).setMessage(R.string.unknown_current_position);
    verify(dialog).setPositiveButton(R.string.ok, null);
    verify(dialog, never()).setNegativeButton(anyInt(), any());
  }

  private static void confirm(MaterialAlertDialogBuilder dialog)
  {
    final ArgumentCaptor<DialogInterface.OnClickListener> action =
        ArgumentCaptor.forClass(DialogInterface.OnClickListener.class);
    verify(dialog).setNegativeButton(R.string.cancel, null);
    verify(dialog).setPositiveButton(eq(R.string.ok), action.capture());
    action.getValue().onClick(null, DialogInterface.BUTTON_POSITIVE);
  }
}
