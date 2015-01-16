#pragma once


namespace dlg_settings
{
  /// @note Do not change numeric values, order and leave DlgCount last.
  //@{
  enum DialogT { FacebookDlg = 0, AppStore, DlgCount };
  enum ResultT { OK = 0, Later, Never };
  //@}

  void EnterBackground(double elapsed);

  bool ShouldShow(DialogT dlg);
  void SaveResult(DialogT dlg, ResultT res);
}
