package com.mapswithme.maps.promo.request;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import com.mapswithme.util.Utils;

public abstract class PromoActivationRequest
{
  protected RequestListener mRequestListener;

  /**
   * Reads and interprets response from server.
   *
   * @param is {@link InputStream} from server.
   * @return true if activation successful, false otherwise
   */
  public abstract boolean parseResponse(InputStream is);

  /**
   *  Creates {@link URL} for specific server using passed promocode.
   *
   * @param promoCode promocode to activate
   * @return {@link URL} using promocode
   * @throws MalformedURLException
   */
  public abstract URL createUrl(String promoCode) throws MalformedURLException;

  /**
   * Template method for client's code.
   * Uses {@link PromoActivationRequest#createUrl(String)}
   * and {@link PromoActivationRequest#doRequest(URL)}
   * methods.
   *
   * @param promoCode
   * @throws Exception
   */
  public void doRequst(String promoCode) throws Exception
  {
    doRequest(createUrl(promoCode));
  }

  public void setRequestListener(RequestListener listener)
  {
    mRequestListener = listener;
  }

  protected void doRequest(URL url)
  {
    HttpURLConnection urlConnection = null;
    InputStream in = null;
    try
    {
      urlConnection= (HttpURLConnection) url.openConnection();
      in = new BufferedInputStream(urlConnection.getInputStream());

      boolean success = parseResponse(in);
      if (success)
        mRequestListener.onSuccess();
      else
        mRequestListener.onFailure();

    }
    catch (IOException e)
    {
      e.printStackTrace();
      mRequestListener.onError(e);
    }
    finally
    {
      Utils.closeStream(in);
      urlConnection.disconnect();
    }

  }

  public interface RequestListener
  {
    public void onSuccess();
    public void onFailure();
    public void onError(Exception ex);
  }
}
