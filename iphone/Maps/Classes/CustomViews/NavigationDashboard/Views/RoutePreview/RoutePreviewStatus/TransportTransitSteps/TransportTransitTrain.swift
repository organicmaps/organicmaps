final class TransportTransitTrain: TransportTransitCell {
  enum Config {
    static let backgroundCornerRadius: CGFloat = 4
    static var labelTextColor: UIColor { return .white }
    static let labelTextFont = UIFont.bold12()!
    static let labelTrailing: CGFloat = 4
  }

  @IBOutlet private weak var background: UIView! {
    didSet {
      background.layer.cornerRadius = Config.backgroundCornerRadius
    }
  }

  @IBOutlet private weak var image: UIImageView!
  @IBOutlet private weak var label: UILabel! {
    didSet {
      label.textColor = Config.labelTextColor
      label.font = Config.labelTextFont
    }
  }

  @IBOutlet private weak var labelTrailing: NSLayoutConstraint! {
    didSet {
      labelTrailing.constant = Config.labelTrailing
    }
  }

  @IBOutlet private var emptyNumberTrailingOffset: NSLayoutConstraint!

  override class func estimatedCellSize(step: MWMRouterTransitStepInfo) -> CGSize {
    let defaultSize = super.estimatedCellSize(step: step)
    let labelSize = step.number.size(width: CGFloat.greatestFiniteMagnitude, font: Config.labelTextFont, maxNumberOfLines: 1)
    return CGSize(width: defaultSize.width + labelSize.width + Config.labelTrailing, height: defaultSize.height)
  }

  override func config(step: MWMRouterTransitStepInfo) {
    switch step.type {
    case .intermediatePoint: fallthrough
    case .pedestrian: fatalError()
    case .train: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_train")
    case .subway: fallthrough
    case .lightRail: fallthrough
    case .monorail: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_metro")
    }
    background.backgroundColor = step.color

    emptyNumberTrailingOffset.isActive = step.number.isEmpty
    label.isHidden = step.number.isEmpty
    label.text = step.number
  }
}
