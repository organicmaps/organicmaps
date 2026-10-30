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
  @IBOutlet private var imageAspectRatio: NSLayoutConstraint!
  @IBOutlet private var imageLeading: NSLayoutConstraint!
  private lazy var hiddenImageWidth = image.widthAnchor.constraint(equalToConstant: 0)

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
    estimatedCellSize(number: step.number)
  }

  static func estimatedCellSize(number: String, isContinuation: Bool = false) -> CGSize {
    let defaultSize = TransportTransitCell.Config.cellSize
    let labelSize = number.size(width: CGFloat.greatestFiniteMagnitude, font: Config.labelTextFont, maxNumberOfLines: 1)
    let leadingWidth = isContinuation ? Config.labelTrailing : defaultSize.width
    return CGSize(width: leadingWidth + labelSize.width + Config.labelTrailing, height: defaultSize.height)
  }

  static func numberGroups(_ number: String, maximumWidth: CGFloat, firstRowWidth: CGFloat,
                           hasFollowingStep: Bool) -> [String] {
    var remaining = ArraySlice(number.components(separatedBy: ", "))
    var groups = [String]()
    let separatorWidth = TransportTransitFlowLayout.Config.separatorSize.width
    var rowWidth = firstRowWidth

    while !remaining.isEmpty {
      let isContinuation = !groups.isEmpty
      func measuredWidth(_ text: String) -> CGFloat {
        estimatedCellSize(number: text, isContinuation: isContinuation).width
      }
      let availableWidth = rowWidth - (hasFollowingStep ? separatorWidth : 0)

      let firstNumber = remaining.first! + (remaining.count > 1 ? "," : "")
      if rowWidth < maximumWidth, measuredWidth(firstNumber) > rowWidth {
        rowWidth = maximumWidth
        continue
      }

      var current = remaining.removeFirst()
      while let next = remaining.first {
        let candidate = current + ", " + next
        let suffix = remaining.count > 1 ? "," : ""
        let limit = remaining.count == 1 ? availableWidth : rowWidth
        guard measuredWidth(candidate + suffix) <= limit else { break }
        current = candidate
        remaining.removeFirst()
      }
      let suffix = !remaining.isEmpty && measuredWidth(current + ",") <= rowWidth ? "," : ""
      groups.append(current + suffix)
      rowWidth = maximumWidth
    }
    return groups
  }

  override func config(step: MWMRouterTransitStepInfo) {
    config(step: step, isContinuation: false)
  }

  func config(step: MWMRouterTransitStepInfo, number: String? = nil, isContinuation: Bool) {
    let number = number ?? step.number!
    imageAspectRatio.isActive = false
    hiddenImageWidth.isActive = isContinuation
    imageAspectRatio.isActive = !isContinuation
    imageLeading.constant = isContinuation ? Config.labelTrailing : 0
    image.isHidden = isContinuation
    switch step.type {
    case .intermediatePoint, .pedestrian, .ruler: fatalError()
    case .train: image.image = UIImage.ic20PxRoutePlanningTrain
    case .subway: image.image = UIImage.ic20PxRoutePlanningMetro
    case .lightRail: image.image = UIImage.ic20PxRoutePlanningLightrail
    case .monorail: image.image = UIImage.ic20PxRoutePlanningMonorail
    case .tram, .cableTram: image.image = UIImage.ic20PxRoutePlanningTram
    case .bus: image.image = UIImage.ic20PxRoutePlanningBus
    case .ferry, .waterService: image.image = UIImage.ic20PxRoutePlanningFerry
    case .aerialLift: image.image = UIImage.ic20PxRoutePlanningAerialLift
    case .funicular: image.image = UIImage.ic20PxRoutePlanningFunicular
    case .airService: image.image = UIImage.ic20PxRoutePlanningAirService
    }
    background.backgroundColor = step.color
    let foreground = Self.foregroundColor(for: step.color)
    image.tintColor = foreground
    label.textColor = foreground

    emptyNumberTrailingOffset.isActive = number.isEmpty
    label.isHidden = number.isEmpty
    label.text = number
  }

  static func foregroundColor(for color: UIColor) -> UIColor {
    // The map brightens dark raw line colors at night, so contrast depends on the badge's actual color.
    var red: CGFloat = 0
    var green: CGFloat = 0
    var blue: CGFloat = 0
    var alpha: CGFloat = 0
    let isRGB = color.getRed(&red, green: &green, blue: &blue, alpha: &alpha)
    precondition(isRGB)
    func linear(_ component: CGFloat) -> CGFloat {
      component <= 0.04045 ? component / 12.92 : pow((component + 0.055) / 1.055, 2.4)
    }
    let luminance = 0.2126 * linear(red) + 0.7152 * linear(green) + 0.0722 * linear(blue)
    let whiteContrast = 1.05 / (luminance + 0.05)
    let blackContrast = (luminance + 0.05) / 0.05
    return whiteContrast >= blackContrast ? .white : .black
  }
}
