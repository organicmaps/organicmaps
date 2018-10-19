@objc(MWMSearchBannerCellDelegate)
protocol SearchBannerCellDelegate: AnyObject {
  func cellDidPressAction(_ cell: SearchBannerCell);
  func cellDidPressClose(_ cell: SearchBannerCell);
}

@objc(MWMSearchBannerCell)
class SearchBannerCell: MWMTableViewCell {

  @objc weak var delegate: SearchBannerCellDelegate?

  @IBAction private func onInstall(_ sender: UIButton) {
    delegate?.cellDidPressAction(self)
  }

  @IBAction private func onClose(_ sender: UIButton) {
    delegate?.cellDidPressClose(self)
  }
}
