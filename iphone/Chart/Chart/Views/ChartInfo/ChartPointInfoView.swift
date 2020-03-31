import UIKit

class ChartPointInfoView: UIView {
  enum Alignment {
    case left
    case right
  }

  let captionLabel = UILabel()
  let distanceLabel = UILabel()
  let altitudeLabel = UILabel()
  let stackView = UIStackView()

  let maskLayer = CAShapeLayer()
  var maskPath: UIBezierPath?

  var arrowY: CGFloat? {
    didSet {
      setNeedsLayout()
    }
  }

  var alignment = Alignment.left {
    didSet {
      updateMask()
    }
  }

  var font: UIFont = UIFont.systemFont(ofSize: 12, weight: .regular) {
    didSet {
      captionLabel.font = font
      distanceLabel.font = font
      altitudeLabel.font = font
    }
  }

  var textColor: UIColor = .lightGray {
    didSet {
      captionLabel.textColor = textColor
      distanceLabel.textColor = textColor
      altitudeLabel.textColor = textColor
    }
  }

  override var backgroundColor: UIColor? {
    didSet {
      maskLayer.fillColor = backgroundColor?.cgColor
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    layer.cornerRadius = 5
    backgroundColor = .clear
    layer.shadowColor = UIColor(white: 0, alpha: 1).cgColor
    layer.shadowOpacity = 0.25
    layer.shadowRadius = 2
    layer.shadowOffset = CGSize(width: 0, height: 2)
    maskLayer.fillColor = backgroundColor?.cgColor
    layer.addSublayer(maskLayer)

    stackView.alignment = .leading
    stackView.axis = .vertical
    stackView.translatesAutoresizingMaskIntoConstraints = false
    addSubview(stackView)

    NSLayoutConstraint.activate([
      stackView.leftAnchor.constraint(equalTo: leftAnchor, constant: 6),
      stackView.topAnchor.constraint(equalTo: topAnchor, constant: 6),
      stackView.rightAnchor.constraint(equalTo: rightAnchor, constant: -6),
      stackView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: -6)
    ])

    stackView.addArrangedSubview(captionLabel)
    stackView.addArrangedSubview(distanceLabel)
    stackView.addArrangedSubview(altitudeLabel)
    stackView.setCustomSpacing(6, after: distanceLabel)

    captionLabel.text = NSLocalizedString("placepage_distance", comment: "") + ":"

    captionLabel.font = font
    distanceLabel.font = font
    altitudeLabel.font = font

    captionLabel.textColor = textColor
    distanceLabel.textColor = textColor
    altitudeLabel.textColor = textColor
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  func set(x: CGFloat, label: String, points: [ChartLineInfo]) {
    distanceLabel.text = label
    altitudeLabel.text = "▲ \(points[0].formattedValue)"
  }

  func update(x: CGFloat, label: String, points: [ChartLineInfo]) {
    distanceLabel.text = label
    altitudeLabel.text = "▲ \(points[0].formattedValue)"
    layoutIfNeeded()
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    let y = arrowY ?? bounds.midY
    let path = UIBezierPath(roundedRect: bounds, cornerRadius: 3)
    let trianglePath = UIBezierPath()
    trianglePath.move(to: CGPoint(x: bounds.maxX, y: y - 3))
    trianglePath.addLine(to: CGPoint(x: bounds.maxX + 5, y: y))
    trianglePath.addLine(to: CGPoint(x: bounds.maxX, y: y + 3))
    trianglePath.close()
    path.append(trianglePath)
    maskPath = path
    updateMask()
  }

  private func updateMask() {
    guard let path = maskPath?.copy() as? UIBezierPath else { return }
    if alignment == .right {
      path.apply(CGAffineTransform.identity.scaledBy(x: -1, y: 1).translatedBy(x: -bounds.width, y: 0))
    }
    maskLayer.path = path.cgPath
    layer.shadowPath = path.cgPath
  }
}
