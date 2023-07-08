final class ValueStepperView: UIControl {
  var minValue = 0 {
    didSet {
      guard minValue <= maxValue else { fatalError() }
      value = max(value, minValue)
    }
  }

  var maxValue = 100 {
    didSet {
      guard maxValue >= minValue else { fatalError() }
      value = min(value, maxValue)
    }
  }

  var value = 0 {
    didSet {
      guard value <= maxValue && value >= minValue else { fatalError() }
      minusButton.isEnabled = value > minValue
      plusButton.isEnabled = value < maxValue
      valueLabel.text = "\(value)"
    }
  }

  let minusButton = MWMButton(type: .custom)
  let plusButton = MWMButton(type: .custom)
  let valueLabel = UILabel()

  private var viewConstraints: [NSLayoutConstraint]!

  override init(frame: CGRect) {
    super.init(frame: frame)
    configure()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    configure()
  }

  private func configure() {
    addSubview(minusButton)
    addSubview(valueLabel)
    addSubview(plusButton)
    minusButton.translatesAutoresizingMaskIntoConstraints = false
    valueLabel.translatesAutoresizingMaskIntoConstraints = false
    plusButton.translatesAutoresizingMaskIntoConstraints = false

    valueLabel.textAlignment = .center
    minusButton.isEnabled = false

    minusButton.setImage(UIImage(named: "ic_booking_minus"), for: .normal)
    plusButton.setImage(UIImage(named: "ic_booking_plus"), for: .normal)
    valueLabel.text = "\(value)"

    minusButton.addTarget(self, action: #selector(onMinus(_:)), for: .touchUpInside)
    plusButton.addTarget(self, action: #selector(onPlus(_:)), for: .touchUpInside)

    NSLayoutConstraint.activate([
      minusButton.leadingAnchor.constraint(equalTo: leadingAnchor),
      valueLabel.leadingAnchor.constraint(equalTo: minusButton.trailingAnchor),
      plusButton.leadingAnchor.constraint(equalTo: valueLabel.trailingAnchor),
      plusButton.trailingAnchor.constraint(equalTo: trailingAnchor),
      valueLabel.topAnchor.constraint(equalTo: topAnchor),
      valueLabel.bottomAnchor.constraint(equalTo: bottomAnchor),
      minusButton.topAnchor.constraint(equalTo: topAnchor),
      minusButton.bottomAnchor.constraint(equalTo: bottomAnchor),
      plusButton.topAnchor.constraint(equalTo: topAnchor),
      plusButton.bottomAnchor.constraint(equalTo: bottomAnchor),
      minusButton.widthAnchor.constraint(equalTo: minusButton.heightAnchor, multiplier: 1),
      plusButton.widthAnchor.constraint(equalTo: plusButton.heightAnchor, multiplier: 1)
    ])
  }

  @objc func onMinus(_ sender: UIButton) {
    value -= 1
    sendActions(for: .valueChanged)
  }

  @objc func onPlus(_ sender: UIButton) {
    value += 1
    sendActions(for: .valueChanged)
  }
}
