@objc(MWMPPHotelDescriptionCell)
final class PPHotelDescriptionCell: MWMTableViewCell
{
  private let kMaximumDescriptionHeight: CGFloat = 60

  @IBOutlet private var compactModeConstraints: [NSLayoutConstraint]!
  @IBOutlet private weak var descriptionText: UILabel!
  @IBOutlet private weak var buttonZeroHeight: NSLayoutConstraint!
  @IBOutlet private weak var button: UIButton!
  private weak var updateDelegate: MWMPlacePageCellUpdateProtocol?

  func config(with description: String, delegate: MWMPlacePageCellUpdateProtocol) {
    descriptionText.text = description;
    descriptionText.sizeToFit()
    updateDelegate = delegate

    let isCompact = descriptionText.height > kMaximumDescriptionHeight;
    if (isCompact) {
      compactModeConstraints.forEach { $0.priority = UILayoutPriorityDefaultHigh }
    } else {
      hideButton()
    }

    setNeedsLayout()
  }

  private func hideButton() {
    button.isHidden = true
    buttonZeroHeight.priority = UILayoutPriorityDefaultHigh
  }

  @IBAction private func tap() {
    compactModeConstraints.forEach { $0.isActive = false }
    hideButton()
    setNeedsLayout()
    UIView.animate(withDuration: kDefaultAnimationDuration, animations: { [weak self] in
      guard let s = self else { return }

      s.layoutIfNeeded()
      s.updateDelegate?.cellUpdated()
    })
  }
}
