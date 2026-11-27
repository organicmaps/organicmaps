final class EstimatesView: UIView {
  private enum Constants {
    static let animationDuration: TimeInterval = AppConstants.defaultAnimationDuration / 2
  }

  enum State: Equatable {
    case none
    case estimates(NSAttributedString)
    case loading
    case error(String)
  }

  private var state: State = .none
  private let estimatesLabel = UILabel()

  init() {
    super.init(frame: .zero)
    layout()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func layout() {
    estimatesLabel.adjustsFontForContentSizeCategory = true
    estimatesLabel.configureSingleLineAutoScaling()
    estimatesLabel.isAccessibilityElement = false
    accessibilityTraits = .staticText

    addSubview(estimatesLabel)

    estimatesLabel.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      estimatesLabel.leadingAnchor.constraint(equalTo: leadingAnchor),
      estimatesLabel.trailingAnchor.constraint(equalTo: trailingAnchor),
      estimatesLabel.topAnchor.constraint(equalTo: topAnchor),
      estimatesLabel.bottomAnchor.constraint(equalTo: bottomAnchor),
    ])
  }

  func setState(_ state: State) {
    guard self.state != state else { return }
    switch state {
    case .none:
      isAccessibilityElement = false
      accessibilityLabel = nil
    case .loading:
      isAccessibilityElement = true
      accessibilityLabel = L("routing_accessibility_calculating")
    case .error(let message):
      isAccessibilityElement = true
      accessibilityLabel = message
    case .estimates(let estimates):
      isAccessibilityElement = true
      accessibilityLabel = estimates.string
    }
    UIView.transition(with: self,
                      duration: Constants.animationDuration,
                      options: .transitionCrossDissolve,
                      animations: { [weak self] in
                        guard let self else { return }
                        switch state {
                        case .none:
                          self.estimatesLabel.alpha = 0.0
                          self.estimatesLabel.text = nil
                          self.estimatesLabel.attributedText = nil
                        case .error(let errorMessage):
                          self.estimatesLabel.alpha = 1.0
                          self.estimatesLabel.attributedText = NSAttributedString(string: errorMessage, attributes: [
                            .font: UIFont.semibold16.dynamic,
                            .foregroundColor: UIColor.buttonRed,
                          ])
                        case .estimates(let estimates):
                          self.estimatesLabel.alpha = 1.0
                          self.estimatesLabel.text = nil
                          self.estimatesLabel.attributedText = estimates
                        case .loading:
                          self.estimatesLabel.alpha = 0.0
                          self.estimatesLabel.text = nil
                          self.estimatesLabel.attributedText = nil
                        }
                      })
  }
}
