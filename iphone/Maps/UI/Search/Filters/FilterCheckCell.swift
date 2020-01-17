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
      checkInLabel.text = L("booking_filters_check_in").uppercased()
    }
  }

  @IBOutlet weak var checkOutLabel: UILabel! {
    didSet {
      checkOutLabel.text = L("booking_filters_check_out").uppercased()
    }
  }

  @IBOutlet weak var checkIn: UIButton!
  @IBOutlet weak var checkOut: UIButton!
  @IBOutlet private weak var offlineLabel: UILabel! {
    didSet {
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

  override func awakeFromNib() {
    super.awakeFromNib()
    isSeparatorHidden = true
  }
}
