
package com.mapswithme.maps.api;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;


// TODO add javadoc with example
public final class MWMResponseReciever extends BroadcastReceiver
{
  // I believe this it not the best approach
  // But we leave it to keep things simple
  static MWMResponseHandler sResponseHandler;

  @Override
  final public void onReceive(Context context, Intent intent)
  {
    if (   MapsWithMeApi.getCallbackAction(context).equals(intent.getAction())
        && sResponseHandler != null)
    {
      sResponseHandler.onResponse(context, MWMResponse.extractFromIntent(context, intent));
      // clean up handler to avoid context-leak
      sResponseHandler = null;
    }
  }

}
