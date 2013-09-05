package com.mapswithme.yopme.util;

public class EglInitializeException extends Exception
{
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public EglInitializeException(String message)
	{
		super(message);
	}
	
	public EglInitializeException(String message, int errorCode)
	{
		super(message);
		mErrorCode = errorCode;
	}
	
	public int GeEglErrorCode()
	{
		return mErrorCode;
	}
	
	private int mErrorCode;
}
