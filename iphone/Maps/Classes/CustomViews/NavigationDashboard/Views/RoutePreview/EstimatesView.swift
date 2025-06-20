final class EstimatesView: UIView {
  private enum Constants {
    static let animationDuration: TimeInterval = kDefaultAnimationDuration / 2
  }

  enum State {
    case estimates(NSAttributedString)
    case loading
    case error(String)
  }

  private let estimatesLabel = UILabel()

  init() {
    super.init(frame: .zero)
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func layout() {
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
    UIView.transition(with: self,
                      duration: Constants.animationDuration,
                      options: .transitionCrossDissolve,
                      animations: { [weak self] in
      guard let self else { return }
      switch state {
      case .error(let errorMessage):
        self.estimatesLabel.alpha = 1.0
        self.estimatesLabel.attributedText = NSAttributedString(string: errorMessage, attributes: [
          .font: Fonts().semibold16,
          .foregroundColor: UIColor.buttonRed()
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
