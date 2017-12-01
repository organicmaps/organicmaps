@objc(MWMFilterCheckCellDelegate)
protocol FilterCheckCellDelegate {
  func checkCellButtonTap(_ button: UIButton)
}

@objc(MWMFilterCheckCell)
final class FilterCheckCell: MWMTableViewCell {
  @IBOutlet private var checkButtons: [UIButton]!
  @IBOutlet private var checkLabels: [UILabel]!
  @IBOutlet weak var checkInLabel: UILabel! {
    didSet {
      checkInLabel.text = L("booking_filters_check_in")
    }
  }

  @IBOutlet weak var checkOutLabel: UILabel! {
    didSet {
      checkOutLabel.text = L("booking_filters_check_out")
    }
  }

  @IBOutlet weak var checkIn: UIButton!
  @IBOutlet weak var checkOut: UIButton!
  @IBOutlet private weak var offlineLabel: UILabel! {
    didSet {
      offlineLabel.font = UIFont.regular12()
      offlineLabel.textColor = UIColor.red
      offlineLabel.text = L("booking_filters_offline")
    }
  }

  @IBOutlet private weak var offlineLabelBottomOffset: NSLayoutConstraint!

  @objc var isOffline = false {
    didSet {
      offlineLabel.isHidden = !isOffline
      offlineLabelBottomOffset.priority = isOffline ? .defaultHigh : .defaultLow
      checkLabels.forEach { $0.isEnabled = !isOffline }
      checkButtons.forEach { $0.isEnabled = !isOffline }
    }
  }

  @objc weak var delegate: FilterCheckCellDelegate?

  @IBAction private func tap(sender: UIButton!) {
    delegate?.checkCellButtonTap(sender)
  }

  fileprivate func setupButton(_ button: UIButton) {
    button.setTitleColor(UIColor.blackPrimaryText(), for: .normal)
    button.setTitleColor(UIColor.blackDividers(), for: .disabled)
    button.setBackgroundColor(UIColor.white(), for: .normal)
    let label = button.titleLabel!
    label.textAlignment = .natural
    label.font = UIFont.regular14()
    let layer = button.layer
    layer.cornerRadius = 4
    layer.borderWidth = 1
    layer.borderColor = UIColor.blackDividers().cgColor
  }

  @objc func refreshButtonsAppearance() {
    checkButtons.forEach { setupButton($0) }
  }

  fileprivate func setupLabel(_ label: UILabel) {
    label.font = UIFont.bold12()
    label.textColor = UIColor.blackSecondaryText()
  }

  @objc func refreshLabelsAppearance() {
    checkLabels.forEach { setupLabel($0) }
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    isSeparatorHidden = true
    backgroundColor = UIColor.clear
  }
}
