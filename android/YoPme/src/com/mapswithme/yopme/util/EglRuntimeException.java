package com.mapswithme.yopme.util;

public class EglRuntimeException extends RuntimeException
{
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public EglRuntimeException(String message, int errorCode)
	{
		super(message);
		mErrorCode = errorCode;
	}
	
	public int GetErrorCode()
	{
		return mErrorCode;
	}
	
	private int mErrorCode;
}
