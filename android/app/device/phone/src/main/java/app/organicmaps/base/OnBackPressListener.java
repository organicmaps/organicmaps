package app.organicmaps.base;

public interface OnBackPressListener
{
  /**
   * Fragment tries to process back button press.
   *
   * @return true, if back was processed & fragment shouldn't be closed. false otherwise.
   */
  boolean onBackPressed();
}
