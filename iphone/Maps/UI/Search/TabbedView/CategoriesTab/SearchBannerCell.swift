@objc(MWMSearchBannerCellDelegate)
protocol SearchBannerCellDelegate: AnyObject {
  func cellDidPressAction(_ cell: SearchBannerCell);
  func cellDidPressClose(_ cell: SearchBannerCell);
}

@objc(MWMSearchBannerCell)
class SearchBannerCell: MWMTableViewCell {

  @IBOutlet var taxiImageView: UIImageView!
  @objc weak var delegate: SearchBannerCellDelegate?

  override func awakeFromNib() {
    super.awakeFromNib()
    taxiImageView.mwm_name = "ic_rutaxi"
  }
  
  @IBAction private func onInstall(_ sender: UIButton) {
    delegate?.cellDidPressAction(self)
  }

  @IBAction private func onClose(_ sender: UIButton) {
    delegate?.cellDidPressClose(self)
  }
}
