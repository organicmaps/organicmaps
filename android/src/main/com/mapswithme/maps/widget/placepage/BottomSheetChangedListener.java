package com.mapswithme.maps.widget.placepage;

public interface BottomSheetChangedListener
{
  void onSheetHidden();
  void onSheetDirectionIconChange();
  void onSheetDetailsOpened();
  void onSheetCollapsed();
  void onSheetSliding(int top);
  void onSheetSlideFinish();
}
