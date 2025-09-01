package app.organicmaps.sync.nextcloud;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.TypedValue;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import androidx.annotation.MainThread;
import androidx.annotation.WorkerThread;
import androidx.appcompat.app.AlertDialog;
import app.organicmaps.R;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.net.MalformedURLException;
import java.net.URL;

public class UrlInputDialog
{
  public interface OnSuccessfulConnection
  {
    @MainThread
    void onSuccessfulConnection(String loginUrl, PollParams pollParams);
  }

  private final Context context;
  private AlertDialog mDialog;
  private EditText mEditText;
  private ProgressBar mProgressBar;
  private TextView mErrorTv;
  private Button mNextButton;

  public UrlInputDialog(Context context)
  {
    this.context = context;
    createDialog();
  }

  @MainThread
  public void show(OnSuccessfulConnection callback)
  {
    mDialog.setOnShowListener(dialogInterface -> setupButtonClickListener(callback));
    mDialog.show();
  }

  private void createDialog()
  {
    mDialog = new MaterialAlertDialogBuilder(context, getDialogTheme())
                  .setTitle(context.getString(R.string.enter_nextcloud_url))
                  .setNegativeButton(R.string.cancel, (dialogInterface, i) -> dialogInterface.dismiss())
                  .setPositiveButton(R.string.next_button, null)
                  .create();

    mDialog.setView(View.inflate(context, R.layout.item_url_input, null));
  }

  private int getDialogTheme()
  {
    // This theme is needed because using R.style.MwmTheme_AlertDialog causes a blank
    // underlay to appear below the copy/paste/select menu on the EditText, probably due to having
    // a background defined.
    TypedValue resolvedId = new TypedValue();
    boolean foundTheme = context.getTheme().resolveAttribute(R.attr.alertDialogThemeWide, resolvedId, true);
    return foundTheme ? resolvedId.data : R.style.MwmTheme_AlertDialog;
  }

  private void setupButtonClickListener(OnSuccessfulConnection callback)
  {
    initializeViews();
    mNextButton.setOnClickListener(view -> handleNextButtonClick(callback));
  }

  private void initializeViews()
  {
    mEditText = mDialog.findViewById(R.id.et_url_input);
    mProgressBar = mDialog.findViewById(R.id.pb_url_input);
    mErrorTv = mDialog.findViewById(R.id.tv_url_input_error);
    mNextButton = mDialog.getButton(AlertDialog.BUTTON_POSITIVE);
  }

  private void handleNextButtonClick(final OnSuccessfulConnection callback)
  {
    String input = mEditText.getText().toString();
    try
    {
      // Prepend "https://" if scheme (https://datatracker.ietf.org/doc/html/rfc3986#section-3.1) not specified
      if (!input.matches("^[a-zA-Z][a-zA-Z0-9+.-]*://.*"))
      {
        input = "https://" + input;
        mEditText.setText(input);
      }

      final URL url = new URL(input);
      String baseUrl = url.getProtocol() + "://" + url.getHost();
      int port = url.getPort();
      if (port != -1)
        baseUrl += ":" + port;
      final String connectUrl = baseUrl + "/index.php/login/v2";

      setLoadingState(true);

      ThreadPool.getWorker().execute(() -> attemptConnection(connectUrl, callback));
    }
    catch (MalformedURLException e)
    {
      showError(context.getString(R.string.hint_enter_valid_url));
    }
  }

  @WorkerThread
  private void attemptConnection(final String connectUrl, final OnSuccessfulConnection callback)
  {
    ConnectionRequest.make(context, connectUrl, new ConnectionRequest.ConnectionRequestCallback() {
      @Override
      public void onSuccess(String loginUrl, PollParams pollParams)
      {
        new Handler(Looper.getMainLooper()).post(() -> {
          mDialog.dismiss();
          callback.onSuccessfulConnection(loginUrl, pollParams);
        });
      }
      @Override
      public void onError(String message)
      {
        new Handler(Looper.getMainLooper()).post(() -> {
          showError(message);
          setLoadingState(false);
        });
      }
    });
  }

  private void setLoadingState(boolean isLoading)
  {
    mProgressBar.setVisibility(isLoading ? View.VISIBLE : View.GONE);
    mEditText.setEnabled(!isLoading);
    mNextButton.setEnabled(!isLoading);
    if (isLoading)
      mErrorTv.setVisibility(View.GONE);
  }

  private void showError(String errorMessage)
  {
    mErrorTv.setText(errorMessage);
    mErrorTv.setVisibility(View.VISIBLE);
  }
}
