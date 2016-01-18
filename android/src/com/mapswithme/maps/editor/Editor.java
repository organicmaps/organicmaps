package com.mapswithme.maps.editor;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.bookmarks.data.Metadata;


/**
 * Edits active(selected on the map) MapObjects(aka UserMark in core).
 * All the methods apply to currently active objects.
 */
public final class Editor
{
  private Editor() {}

  public static boolean hasEditableAttributes()
  {
    return Editor.nativeGetEditableMetadata().length != 0 ||
           Editor.nativeIsAddressEditable() ||
           Editor.nativeIsNameEditable();
  }

  public static native @NonNull int[] nativeGetEditableMetadata();

  public static native void nativeSetMetadata(int type, String value);

  public static native void nativeEditFeature(String street, String houseNumber);

  public static native boolean nativeIsAddressEditable();

  public static native boolean nativeIsNameEditable();

  public static native void nativeSetName(String name);
}
