package app.organicmaps.util.bottomsheet;

public class MenuBottomSheetItem
{
  public final int titleRes;
  // Overrides titleRes when non-null (untranslated experimental entries).
  public final String title;
  public final int iconRes;
  public final int badgeCount;
  public final OnClickListener onClickListener;

  public MenuBottomSheetItem(int titleRes, int iconRes, OnClickListener onClickListener)
  {
    this.titleRes = titleRes;
    this.title = null;
    this.iconRes = iconRes;
    this.badgeCount = 0;
    this.onClickListener = onClickListener;
  }

  public MenuBottomSheetItem(String title, int iconRes, OnClickListener onClickListener)
  {
    this.titleRes = 0;
    this.title = title;
    this.iconRes = iconRes;
    this.badgeCount = 0;
    this.onClickListener = onClickListener;
  }

  public MenuBottomSheetItem(int titleRes, int iconRes, int badgeCount, OnClickListener onClickListener)
  {
    this.titleRes = titleRes;
    this.title = null;
    this.iconRes = iconRes;
    this.badgeCount = badgeCount;
    this.onClickListener = onClickListener;
  }

  public interface OnClickListener
  {
    void onClick();
  }
}
