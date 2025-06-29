package app.organicmaps;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.res.ConfigurationHelper;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.MapRenderingListener;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.util.log.Logger;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public class MapFragment extends BaseMwmFragment implements View.OnTouchListener, SurfaceHolder.Callback
{
  private static final String TAG = MapFragment.class.getSimpleName();
  private final Map mMap = new Map(DisplayType.Device);

  public void updateCompassOffset(int offsetX, int offsetY)
  {
    mMap.updateCompassOffset(requireContext(), offsetX, offsetY, true);
  }

  public void updateBottomWidgetsOffset(int offsetX, int offsetY)
  {
    mMap.updateBottomWidgetsOffset(requireContext(), offsetX, offsetY);
  }

  public void updateMyPositionRoutingOffset(int offsetY)
  {
    mMap.updateMyPositionRoutingOffset(offsetY);
  }

  public void destroySurface(boolean activityIsChangingConfigurations)
  {
    mMap.onSurfaceDestroyed(activityIsChangingConfigurations, isAdded());
  }

  public boolean isContextCreated()
  {
    return mMap.isContextCreated();
  }

  @Override
  public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder)
  {
    Logger.d(TAG);
    int densityDpi;

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
      densityDpi = ConfigurationHelper.getDensityDpi(requireContext().getResources());
    else
      densityDpi = getDensityDpiOld();

    mMap.onSurfaceCreated(requireContext(), surfaceHolder.getSurface(), surfaceHolder.getSurfaceFrame(), densityDpi);
  }

  @Override
  public void surfaceChanged(SurfaceHolder surfaceHolder, int format, int width, int height)
  {
    Logger.d(TAG);
    mMap.onSurfaceChanged(requireContext(), surfaceHolder.getSurface(), surfaceHolder.getSurfaceFrame(),
                          surfaceHolder.isCreating());
  }

  @Override
  public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder)
  {
    Logger.d(TAG);
    mMap.onSurfaceDestroyed(requireActivity().isChangingConfigurations(), true);
  }

  @Override
  public void onAttach(Context context)
  {
    Logger.d(TAG);
    super.onAttach(context);
    mMap.setMapRenderingListener((MapRenderingListener) context);
    mMap.setCallbackUnsupported(this::reportUnsupported);
  }

  @Override
  public void onDetach()
  {
    Logger.d(TAG);
    super.onDetach();
    mMap.setMapRenderingListener(null);
    mMap.setCallbackUnsupported(null);
  }

  @Override
  public void onCreate(Bundle b)
  {
    Logger.d(TAG);
    super.onCreate(b);
    setRetainInstance(true);
    boolean launchByDeepLink = false;
    Bundle args = getArguments();
    if (args != null)
      launchByDeepLink = args.getBoolean(Map.ARG_LAUNCH_BY_DEEP_LINK);
    mMap.onCreate(launchByDeepLink);
  }

  @Override
  public void onStart()
  {
    Logger.d(TAG);
    super.onStart();
    mMap.onStart();
  }

  @Override
  public void onStop()
  {
    Logger.d(TAG);
    super.onStop();
    mMap.onStop();
  }

  @Override
  public void onPause()
  {
    Logger.d(TAG);
    super.onPause();
    mMap.onPause(requireContext());
  }

  @Override
  public void onResume()
  {
    Logger.d(TAG);
    super.onResume();
    mMap.onResume();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    Logger.d(TAG);
    final View view = inflater.inflate(R.layout.fragment_map, container, false);
    final SurfaceView mSurfaceView = view.findViewById(R.id.map_surfaceview);
    mSurfaceView.getHolder().addCallback(this);
    return view;
  }

  @Override
  public boolean onTouch(View view, MotionEvent event)
  {
    int action = event.getActionMasked();
    int pointerIndex = event.getActionIndex();
    switch (action)
    {
      case MotionEvent.ACTION_POINTER_UP -> action = Map.NATIVE_ACTION_UP;
      case MotionEvent.ACTION_UP ->
      {
        action = Map.NATIVE_ACTION_UP;
        pointerIndex = 0;
      }
      case MotionEvent.ACTION_POINTER_DOWN -> action = Map.NATIVE_ACTION_DOWN;
      case MotionEvent.ACTION_DOWN ->
      {
        action = Map.NATIVE_ACTION_DOWN;
        pointerIndex = 0;
      }
      case MotionEvent.ACTION_MOVE ->
      {
        action = Map.NATIVE_ACTION_MOVE;
        pointerIndex = Map.INVALID_POINTER_MASK;
      }
      case MotionEvent.ACTION_CANCEL -> action = Map.NATIVE_ACTION_CANCEL;
    }
    Map.onTouch(action, event, pointerIndex);
    return true;
  }

  public void notifyOnSurfaceDestroyed(@NonNull Runnable task)
  {
    mMap.onSurfaceDestroyed(false, true);
    task.run();
  }

  private void reportUnsupported()
  {
    new MaterialAlertDialogBuilder(requireContext(), R.style.MwmTheme_AlertDialog)
        .setMessage(R.string.unsupported_phone)
        .setCancelable(false)
        .setPositiveButton(R.string.close, (dlg, which) -> requireActivity().moveTaskToBack(true))
        .show();
  }

  private int getDensityDpiOld()
  {
    final DisplayMetrics metrics = new DisplayMetrics();
    requireActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
    return metrics.densityDpi;
  }
}
