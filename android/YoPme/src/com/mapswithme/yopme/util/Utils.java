package com.mapswithme.yopme.util;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.text.MessageFormat;

import android.util.Log;

public class Utils
{
  @SuppressWarnings("unchecked")
  public static <T> T createInvocationLogger(final Class<T> clazz, final String logTag)
  {
    final InvocationHandler handler = new InvocationHandler()
    {
      @Override
      public Object invoke(Object proxy, Method method, Object[] args) throws Throwable
      {
        Log.d(logTag,
            MessageFormat.format("Called {0} with {1}", method, args));

        return null;
      }
    };

    return (T) Proxy.newProxyInstance(clazz.getClassLoader(),
        new Class<?>[] { clazz }, handler);
  }

  private Utils() {}
}
