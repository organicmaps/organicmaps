package app.organicmaps.widget.placepage;

public interface BottomSheetChangedListener
{
  void onSheetHidden();
  void onSheetDetailsOpened();
  void onSheetCollapsed();
  void onSheetSliding(int top);
  void onSheetSlideFinish();
}
