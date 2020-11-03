protocol SearchBannerCellDelegate: AnyObject {
  func cellDidPressAction(_ cell: SearchBannerCell);
  func cellDidPressClose(_ cell: SearchBannerCell);
}

class SearchBannerCell: MWMTableViewCell {
  @IBOutlet var iconView: UIImageView!
  @IBOutlet var labelView: UILabel!
  @IBOutlet var buttonView: UIButton!
  
  private weak var delegate: SearchBannerCellDelegate?
  
  func configure(icon: String, label: String, buttonText: String, delegate: SearchBannerCellDelegate?) {
    iconView.mwm_name = icon
    labelView.text  = label
    buttonView.localizedText = buttonText
    self.delegate = delegate
  }
  
  @IBAction private func onInstall(_ sender: UIButton) {
    delegate?.cellDidPressAction(self)
  }

  @IBAction private func onClose(_ sender: UIButton) {
    delegate?.cellDidPressClose(self)
  }
}
