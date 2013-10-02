package com.mapswithme.yopme.util;

public class EglOperationException extends RuntimeException
{
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public EglOperationException(String message, int errorCode)
	{
		super(message);
		mErrorCode = errorCode;
	}
	
	public int getEglErrorCode()
	{
		return mErrorCode;
	}
  
  @Override
  public String toString()
  {
    return super.toString() + " EGL code = " + Integer.toHexString(mErrorCode);
  }
	
	private int mErrorCode;
}
