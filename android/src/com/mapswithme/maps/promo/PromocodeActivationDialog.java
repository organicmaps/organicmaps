package com.mapswithme.maps.promo;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.promo.request.PromoActivationRequest;
import com.mapswithme.util.Utils;

import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;

public class PromocodeActivationDialog extends Dialog
                                       implements OnClickListener, PromoActivationRequest.RequestListener
{


  private EditText mPromoText;
  private TextView mErrorText;
  private Button mCancelBtn;
  private Button mActivateBtn;

  private ProgressBar mProgressBar;
  private Handler mHandler;

  // for www.computerbild.de
  private PromoActivationRequest mComputerBildRequest;

  public PromocodeActivationDialog(Context context)
  {
    super(context);

    mComputerBildRequest = new PromoActivationRequest()
    {
      @Override
      public boolean parseResponse(InputStream is)
      {
        final String response = Utils.readStreamAsString(is);
        return "VALID".equalsIgnoreCase(response);
      }

      @Override
      public URL createUrl(String promoCode) throws MalformedURLException
      {
        final String strUrl =  "https://mwm.cbapps.de/" + promoCode;
        return new URL(strUrl);
      }
    };

    mComputerBildRequest.setRequestListener(this);
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    requestWindowFeature(Window.FEATURE_NO_TITLE);

    setContentView(R.layout.dlg_promo_activation);
    setUpView();
    setViewIsWaiting(false);
    hideError();

    mHandler = new Handler();
  }

  private String getString(int id)
  {
    return getContext().getString(id);
  }

  private void setUpView()
  {
    mPromoText = (EditText) findViewById(R.id.txt_promocode);
    mErrorText = (TextView) findViewById(R.id.txt_error);
    mCancelBtn = (Button) findViewById(R.id.btn_cancel);
    mActivateBtn = (Button) findViewById(R.id.btn_activate);
    mProgressBar = (ProgressBar) findViewById(R.id.progress_bar);

    mCancelBtn.setOnClickListener(this);
    mActivateBtn.setOnClickListener(this);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    // Automatically show keyboard on start
    mPromoText.setOnFocusChangeListener(new View.OnFocusChangeListener()
    {
      @Override
      public void onFocusChange(View v, boolean hasFocus)
      {
        if (hasFocus)
        {
          getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        }
      }
    });
  }

  private void setViewIsWaiting(boolean isWaiting)
  {
    mProgressBar.setVisibility(isWaiting ? View.VISIBLE : View.INVISIBLE);
    mActivateBtn.setEnabled(!isWaiting);
    mCancelBtn.setEnabled(!isWaiting);
    mPromoText.setEnabled(!isWaiting);
    setCancelable(!isWaiting);
  }

  private void hideError()
  {
    mErrorText.setVisibility(View.GONE);
    mErrorText.setText(null);
  }

  private void showError(CharSequence message)
  {
    mErrorText.setText(message);
    mErrorText.setVisibility(View.VISIBLE);
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.btn_activate)
      activateCoupon();
    else if (id == R.id.btn_cancel)
      dismiss();
  }

  private void activateCoupon()
  {
    setViewIsWaiting(true);
    hideError();

    final String promoCode = mPromoText.getText().toString().trim();

    new Thread() {
      @Override
      public void run()
      {
        try
        {
          mComputerBildRequest.doRequst(promoCode);
        }
        catch (Exception e)
        {
          onError(e);
        }
      }
    }.start();
  }

  @Override
  public void onFailure()
  {
    mHandler.post(new Runnable()
    {
      @Override
      public void run()
      {
        setViewIsWaiting(false);
        showError(getString(R.string.promocode_failure));
      }
    });
  }

  @Override
  public void onSuccess()
  {
    mHandler.post(new Runnable()
    {
      @Override
      public void run()
      {
        setViewIsWaiting(false);
        ActivationSettings.setSearchActivated(getContext(), true);
        dismiss();
        Utils.toastShortcut(getContext(), getString(R.string.promocode_success));
      }
    });
  }

  @Override
  public void onError(final Exception ex)
  {
    mHandler.post(new Runnable()
    {
      @Override
      public void run()
      {
        setViewIsWaiting(false);
        showError(getString(R.string.promocode_error));
      }
    });
  }

}
