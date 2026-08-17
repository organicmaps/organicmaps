import UIKit

final class ColorPickerButton: UIButton {
  enum Constants {
    static let buttonSize: CGFloat = 44
    static let circleDiameter: CGFloat = 36
    static let selectionRingInset: CGFloat = 0.18
    static let selectionRingMinLineWidth: CGFloat = 2
    static let selectionRingLineWidthRatio: CGFloat = 0.08
  }

  private let selectionLayer = CAShapeLayer()

  override var intrinsicContentSize: CGSize {
    CGSize(width: Constants.buttonSize, height: Constants.buttonSize)
  }

  override var isSelected: Bool {
    didSet {
      selectionLayer.isHidden = !isSelected
      accessibilityTraits = isSelected ? [.button, .selected] : .button
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    configureSelectionLayer()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateSelectionLayerPath()
  }

  func configure(predefinedColor: PredefinedColor, selected: Bool) {
    let color = BookmarksManager.color(from: predefinedColor)
    setImage(circleImageForColor(color,
                                 frameSize: Constants.buttonSize,
                                 diameter: Constants.circleDiameter), for: .normal)
    accessibilityLabel = predefinedColor.label
    isSelected = selected
  }

  func configure(image: UIImage, label: String, selected: Bool) {
    setImage(image, for: .normal)
    accessibilityLabel = label
    isSelected = selected
  }

  private func configureSelectionLayer() {
    selectionLayer.fillColor = UIColor.clear.cgColor
    selectionLayer.strokeColor = UIColor.white.cgColor
    selectionLayer.isHidden = true
    selectionLayer.zPosition = 1
    layer.addSublayer(selectionLayer)
  }

  private func updateSelectionLayerPath() {
    let diameter = Constants.circleDiameter
    let circleRect = CGRect(x: (bounds.width - diameter) / 2,
                            y: (bounds.height - diameter) / 2,
                            width: diameter,
                            height: diameter)
    let lineWidth = max(Constants.selectionRingMinLineWidth, diameter * Constants.selectionRingLineWidthRatio)
    let ringRect = circleRect.insetBy(dx: diameter * Constants.selectionRingInset,
                                      dy: diameter * Constants.selectionRingInset)
    selectionLayer.frame = bounds
    selectionLayer.lineWidth = lineWidth
    selectionLayer.path = UIBezierPath(ovalIn: ringRect).cgPath
  }
}

private extension PredefinedColor {
  var label: String {
    switch self {
    case .red: L("red")
    case .pink: L("pink")
    case .purple: L("purple")
    case .deepPurple: L("deep_purple")
    case .blue: L("blue")
    case .lightBlue: L("light_blue")
    case .cyan: L("cyan")
    case .teal: L("teal")
    case .green: L("green")
    case .lime: L("lime")
    case .yellow: L("yellow")
    case .orange: L("orange")
    case .deepOrange: L("deep_orange")
    case .brown: L("brown")
    case .gray: L("gray")
    case .blueGray: L("blue_gray")
    case .none, .count: ""
    @unknown default: ""
    }
  }
}
