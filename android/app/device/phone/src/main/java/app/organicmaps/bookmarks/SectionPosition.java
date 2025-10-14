package app.organicmaps.bookmarks;

public class SectionPosition
{
  static final int INVALID_POSITION = -1;

  private final int mSectionIndex;
  private final int mItemIndex;

  SectionPosition(int sectionInd, int itemInd)
  {
    mSectionIndex = sectionInd;
    mItemIndex = itemInd;
  }

  int getSectionIndex()
  {
    return mSectionIndex;
  }

  int getItemIndex()
  {
    return mItemIndex;
  }

  boolean isTitlePosition()
  {
    return mSectionIndex != INVALID_POSITION && mItemIndex == INVALID_POSITION;
  }

  boolean isItemPosition()
  {
    return mSectionIndex != INVALID_POSITION && mItemIndex != INVALID_POSITION;
  }
}
