final class TransportTransitTrain: TransportTransitCell {
  enum Config {
    static let labelTextFont = UIFont.bold12
    static let labelTrailing: CGFloat = 4
  }

  @IBOutlet private var background: UIView! {
    didSet {
      background.layer.setCornerRadius(.buttonSmall)
    }
  }

  @IBOutlet private var image: UIImageView!

  @IBOutlet private var label: UILabel! {
    didSet {
      label.font = Config.labelTextFont
    }
  }

  @IBOutlet private var labelTrailing: NSLayoutConstraint! {
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
    case .intermediatePoint, .pedestrian, .ruler: fatalError()
    case .train: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_train")
    case .subway: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_metro")
    case .lightRail: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_lightrail")
    case .monorail: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_monorail")
    case .tram, .cableTram: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_tram")
    case .bus: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_bus")
    case .ferry, .waterService: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_ferry")
    case .aerialLift: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_aerial_lift")
    case .funicular: image.image = #imageLiteral(resourceName: "ic_20px_route_planning_funicular")
    case .airService: image.image = #imageLiteral(resourceName: "ic_bm_airport").withRenderingMode(.alwaysTemplate)
    }
    background.backgroundColor = step.color
    let foreground = foregroundColor(for: step.color)
    image.tintColor = foreground
    label.textColor = foreground

    emptyNumberTrailingOffset.isActive = step.number.isEmpty
    label.isHidden = step.number.isEmpty
    label.text = step.number
  }

  private func foregroundColor(for color: UIColor) -> UIColor {
    // The map brightens dark raw line colors at night, so contrast depends on the badge's actual color.
    var red: CGFloat = 0
    var green: CGFloat = 0
    var blue: CGFloat = 0
    var alpha: CGFloat = 0
    precondition(color.getRed(&red, green: &green, blue: &blue, alpha: &alpha))
    func linear(_ component: CGFloat) -> CGFloat {
      component <= 0.04045 ? component / 12.92 : pow((component + 0.055) / 1.055, 2.4)
    }
    let luminance = 0.2126 * linear(red) + 0.7152 * linear(green) + 0.0722 * linear(blue)
    let whiteContrast = 1.05 / (luminance + 0.05)
    let blackContrast = (luminance + 0.05) / 0.05
    return whiteContrast >= blackContrast ? .white : .black
  }
}
