final class TransportRuler: TransportTransitCell {
  enum Config {
    static let backgroundCornerRadius: CGFloat = 4
    static var backgroundColor: UIColor { return UIColor.blackOpaque() }
    static var imageColor: UIColor { return UIColor.blackSecondaryText() }
    static var labelTextColor: UIColor { return .black }
    static let labelTextFont = UIFont.bold12()
    static let labelTrailing: CGFloat = 8
  }

  @IBOutlet private weak var background: UIView! {
    didSet {
      background.layer.cornerRadius = Config.backgroundCornerRadius
      background.backgroundColor = Config.backgroundColor
    }
  }

  @IBOutlet private weak var label: UILabel! {
    didSet {
      label.textColor = Config.labelTextColor
      label.font = Config.labelTextFont
    }
  }

  override class func estimatedCellSize(step: MWMRouterTransitStepInfo) -> CGSize {
    let defaultSize = super.estimatedCellSize(step: step)
    let labelText = step.distance + " " + step.distanceUnits;
    let labelSize = labelText.size(width: CGFloat.greatestFiniteMagnitude, font: Config.labelTextFont, maxNumberOfLines: 1)
    return CGSize(width: labelSize.width + Config.labelTrailing, height: defaultSize.height)
  }

  override func config(step: MWMRouterTransitStepInfo) {
    label.isHidden = step.distance.isEmpty && step.distanceUnits.isEmpty
    label.text = step.distance + " " + step.distanceUnits
  }
}
