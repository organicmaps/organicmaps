import UIKit

class BadgeWithNumber: UIView {
  private let label = UILabel()

  var color: UIColor = .systemRed {
    didSet {
      backgroundColor = color
    }
  }

  var number: Int = 0 {
    didSet {
      label.text = "\(number)"
      isHidden = number <= 0
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    setup()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setup()
  }

  private func setup() {
    isUserInteractionEnabled = false
    backgroundColor = .systemRed
    label.textColor = .white
    label.font = UIFont.systemFont(ofSize: 10, weight: .bold)
    label.textAlignment = .center
    label.translatesAutoresizingMaskIntoConstraints = false
    addSubview(label)

    NSLayoutConstraint.activate([
      label.centerXAnchor.constraint(equalTo: centerXAnchor),
      label.centerYAnchor.constraint(equalTo: centerYAnchor),
      label.leadingAnchor.constraint(greaterThanOrEqualTo: leadingAnchor, constant: 4),
      label.trailingAnchor.constraint(lessThanOrEqualTo: trailingAnchor, constant: -4),
      label.topAnchor.constraint(greaterThanOrEqualTo: topAnchor, constant: 2),
      label.bottomAnchor.constraint(lessThanOrEqualTo: bottomAnchor, constant: -2)
    ])

    layer.masksToBounds = true
    isHidden = true
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    layer.cornerRadius = bounds.height / 2
  }

  func badgeAddTo(_ view: UIView, insets: UIEdgeInsets = UIEdgeInsets(top: -4, left: 0, bottom: 0, right: -4)) {
    translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(self)
    NSLayoutConstraint.activate([
      topAnchor.constraint(equalTo: view.topAnchor, constant: insets.top),
      trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -insets.right),
      heightAnchor.constraint(greaterThanOrEqualToConstant: 18),
      widthAnchor.constraint(greaterThanOrEqualTo: heightAnchor)
    ])
  }
}
