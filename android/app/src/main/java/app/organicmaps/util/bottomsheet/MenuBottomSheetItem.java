package app.organicmaps.util.bottomsheet;

public class MenuBottomSheetItem
{
  public final int titleRes;
  public final int iconRes;
  public final int badgeCount;
  public final OnClickListener onClickListener;
  public final OnLongClickListener onLongClickListener;

  public MenuBottomSheetItem(int titleRes, int iconRes, OnClickListener onClickListener)
  {
    this.titleRes = titleRes;
    this.iconRes = iconRes;
    this.badgeCount = 0;
    this.onClickListener = onClickListener;
    this.onLongClickListener = () -> {};
  }

  public MenuBottomSheetItem(int titleRes, int iconRes, OnClickListener onClickListener, OnLongClickListener onLongClickListener)
  {
    this.titleRes = titleRes;
    this.iconRes = iconRes;
    this.badgeCount = 0;
    this.onClickListener = onClickListener;
    this.onLongClickListener = onLongClickListener;
  }

  public MenuBottomSheetItem(int titleRes, int iconRes, int badgeCount, OnClickListener onClickListener)
  {
    this.titleRes = titleRes;
    this.iconRes = iconRes;
    this.badgeCount = badgeCount;
    this.onClickListener = onClickListener;
    this.onLongClickListener = () -> {};
  }

  public MenuBottomSheetItem(int titleRes, int iconRes, int badgeCount, OnClickListener onClickListener, OnLongClickListener onLongClickListener)
  {
    this.titleRes = titleRes;
    this.iconRes = iconRes;
    this.badgeCount = badgeCount;
    this.onClickListener = onClickListener;
    this.onLongClickListener = onLongClickListener;
  }

  public interface OnClickListener
  {
    void onClick();
  }

  public interface OnLongClickListener
  {
    void onLongClick();
  }
}
