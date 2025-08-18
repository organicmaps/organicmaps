final class PlaceholderView: UIView {

  private let activityIndicator: UIActivityIndicatorView?
  private let titleLabel = UILabel()
  private let subtitleLabel = UILabel()
  private let stackView = UIStackView()
  private var keyboardHeight: CGFloat = 0
  private var centerYConstraint: NSLayoutConstraint!
  private var containerModalYTranslation: CGFloat = 0
  private let minOffsetFromTheKeyboardTop: CGFloat = 20
  private let maxOffsetFromTheTop: CGFloat = 100

  init(title: String? = nil, subtitle: String? = nil, hasActivityIndicator: Bool = false) {
    self.activityIndicator = hasActivityIndicator ? UIActivityIndicatorView() : nil
    super.init(frame: .zero)
    setupView(title: title, subtitle: subtitle)
    layoutView()
    setupKeyboardObservers()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  private func setupKeyboardObservers() {
    NotificationCenter.default.addObserver(self,
                                           selector: #selector(keyboardWillShow(_:)),
                                           name: UIResponder.keyboardWillShowNotification,
                                           object: nil)
    NotificationCenter.default.addObserver(self,
                                           selector: #selector(keyboardWillHide(_:)),
                                           name: UIResponder.keyboardWillHideNotification,
                                           object: nil)
  }

  @objc private func keyboardWillShow(_ notification: Notification) {
    if let keyboardFrame = notification.userInfo?[UIResponder.keyboardFrameEndUserInfoKey] as? CGRect {
      keyboardHeight = keyboardFrame.height
      reloadConstraints()
    }
  }

  @objc private func keyboardWillHide(_ notification: Notification) {
    keyboardHeight = 0
    reloadConstraints()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if traitCollection.verticalSizeClass != previousTraitCollection?.verticalSizeClass {
      reloadConstraints()
    }
  }

  private func setupView(title: String?, subtitle: String?) {
    if let activityIndicator = activityIndicator {
      activityIndicator.hidesWhenStopped = true
      activityIndicator.startAnimating()
      if #available(iOS 13.0, *) {
        activityIndicator.style = .medium
      } else {
        activityIndicator.style = .gray
      }
    }

    titleLabel.text = title
    titleLabel.setFontStyle(.medium16, color: .blackPrimary)
    titleLabel.textAlignment = .center

    subtitleLabel.text = subtitle
    subtitleLabel.setFontStyle(.regular14, color: .blackSecondary)
    subtitleLabel.textAlignment = .center
    subtitleLabel.isHidden = subtitle == nil
    subtitleLabel.numberOfLines = 2

    stackView.axis = .vertical
    stackView.alignment = .center
    stackView.spacing = 8
  }

  private func layoutView() {
    if let activityIndicator = activityIndicator {
      stackView.addArrangedSubview(activityIndicator)
    }
    if let title = titleLabel.text, !title.isEmpty {
      stackView.addArrangedSubview(titleLabel)
    }
    if let subtitle = subtitleLabel.text, !subtitle.isEmpty {
      stackView.addArrangedSubview(subtitleLabel)
    }

    addSubview(stackView)
    stackView.translatesAutoresizingMaskIntoConstraints = false

    centerYConstraint = stackView.centerYAnchor.constraint(equalTo: centerYAnchor)
    NSLayoutConstraint.activate([
      stackView.centerXAnchor.constraint(equalTo: centerXAnchor),
      stackView.widthAnchor.constraint(lessThanOrEqualTo: widthAnchor, multiplier: 0.8),
      centerYConstraint
    ])
  }

  private func reloadConstraints() {
    let offset = keyboardHeight > 0 ? max(bounds.height / 2 - keyboardHeight, minOffsetFromTheKeyboardTop + stackView.frame.height) : containerModalYTranslation / 2
    let maxOffset = bounds.height / 2 - maxOffsetFromTheTop
    centerYConstraint.constant = -min(offset, maxOffset)
    UIView.animate(withDuration: kDefaultAnimationDuration, delay: .zero, options: [.beginFromCurrentState, .curveEaseOut]) {
      self.layoutIfNeeded()
    }
  }
}

// MARK: - ModallyPresentedViewController
extension PlaceholderView: ModallyPresentedViewController {
  func presentationFrameDidChange(_ frame: CGRect) {
    self.containerModalYTranslation = frame.origin.y
    reloadConstraints()
  }
}
