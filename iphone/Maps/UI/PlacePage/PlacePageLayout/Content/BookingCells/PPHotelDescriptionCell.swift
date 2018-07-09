@objc(MWMPPHotelDescriptionCell)
final class PPHotelDescriptionCell: MWMTableViewCell {
  private let kMaximumDescriptionHeight: CGFloat = 60

  @IBOutlet private var compactModeConstraints: [NSLayoutConstraint]!
  @IBOutlet private weak var descriptionText: UILabel!
  @IBOutlet private weak var buttonZeroHeight: NSLayoutConstraint!
  @IBOutlet private weak var button: UIButton!
  private weak var updateDelegate: MWMPlacePageCellUpdateProtocol?
  private var isNeedToForceLayout: Bool = false

  @objc func config(with description: String, delegate: MWMPlacePageCellUpdateProtocol) {
    descriptionText.text = description
    descriptionText.sizeToFit()
    updateDelegate = delegate

    isNeedToForceLayout = true
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    if isNeedToForceLayout {
      isNeedToForceLayout = false
      let isCompact = descriptionText.height > kMaximumDescriptionHeight
      if isCompact {
        compactModeConstraints.forEach { $0.priority = UILayoutPriority.defaultHigh }
      }

      hideButton(!isCompact)
    }
  }

  private func hideButton(_ isHidden: Bool = true) {
    button.isHidden = isHidden
    buttonZeroHeight.priority = isHidden ? UILayoutPriority.defaultHigh : UILayoutPriority.defaultLow
  }

  @IBAction private func tap() {
    animateConstraints(animations: {
      self.compactModeConstraints.forEach { $0.priority = UILayoutPriority.defaultLow }
      self.hideButton()
    }, completion: {
      self.updateDelegate?.cellUpdated()
    })
  }
}
