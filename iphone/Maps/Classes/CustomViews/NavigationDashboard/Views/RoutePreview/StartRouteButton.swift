final class StartRouteButton: UIButton {
  private let activityIndicator: UIActivityIndicatorView = {
    if #available(iOS 13.0, *) {
      UIActivityIndicatorView(style: .medium)
    } else {
      UIActivityIndicatorView(style: .white)
    }
  }()
  private let title = L("Start")

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    setStyle(.flatNormalButtonBig)
    setTitle(title, for: .normal)
    activityIndicator.hidesWhenStopped = true
    activityIndicator.translatesAutoresizingMaskIntoConstraints = false
    addSubview(activityIndicator)
    NSLayoutConstraint.activate([
      activityIndicator.centerXAnchor.constraint(equalTo: centerXAnchor),
      activityIndicator.centerYAnchor.constraint(equalTo: centerYAnchor)
    ])
  }

  func setLoading(_ loading: Bool) {
    if loading {
      setTitle(nil, for: .normal)
      isEnabled = false
      activityIndicator.startAnimating()
    } else {
      setTitle(title, for: .normal)
      isEnabled = true
      activityIndicator.stopAnimating()
    }
  }
}
