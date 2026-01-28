final class LoadingOverlayViewController: UIViewController {
  private var activityIndicator: UIActivityIndicatorView = {
    let indicator: UIActivityIndicatorView
    if #available(iOS 13.0, *) {
      indicator = UIActivityIndicatorView(style: .large)
    } else {
      indicator = UIActivityIndicatorView(style: .whiteLarge)
    }
    indicator.color = .white
    indicator.startAnimating()
    indicator.translatesAutoresizingMaskIntoConstraints = false
    return indicator
  }()

  override func viewDidLoad() {
    super.viewDidLoad()
    view.backgroundColor = UIColor.black.withAlphaComponent(0.3)
    setupViews()
  }

  private func setupViews() {
    view.addSubview(activityIndicator)
    NSLayoutConstraint.activate([
      activityIndicator.centerXAnchor.constraint(equalTo: view.centerXAnchor),
      activityIndicator.centerYAnchor.constraint(equalTo: view.centerYAnchor)
    ])
    view.isUserInteractionEnabled = false
  }
}
