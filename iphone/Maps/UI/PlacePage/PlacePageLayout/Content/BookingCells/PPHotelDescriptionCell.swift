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
    }

    hideButton(!isCompact)
    setNeedsLayout()
  }

  private func hideButton(_ isHidden:Bool = true) {
    button.isHidden = isHidden
    buttonZeroHeight.priority = isHidden ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow
  }

  @IBAction private func tap() {
    compactModeConstraints.forEach { $0.priority = UILayoutPriorityDefaultLow }
    hideButton()
    setNeedsLayout()
    UIView.animate(withDuration: kDefaultAnimationDuration, animations: { [weak self] in
      guard let s = self else { return }

      s.layoutIfNeeded()
      s.updateDelegate?.cellUpdated()
    })
  }
}
