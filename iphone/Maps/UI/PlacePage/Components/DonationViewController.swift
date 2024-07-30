final class DonationViewController: UIViewController {

  private enum Constants {
    static let cornerRadius: CGFloat = 5
    static let spacing: CGFloat = 10
    static let titleTopPadding: CGFloat = 20
    static let titleLeadingPadding: CGFloat = 12
    static let titleTrailingPadding: CGFloat = 10
    static let closeButtonSize: CGFloat = 28
    static let closeButtonTrailingPadding: CGFloat = -12
    static let closeButtonTopPadding: CGFloat = 12
    static let stackViewTopPadding: CGFloat = 12
    static let buttonSpacing: CGFloat = 10
    static let subtitleButtonTopPadding: CGFloat = 8
    static let subtitleButtonBottomPadding: CGFloat = -8
  }

  private let titleLabel = UILabel()
  private let closeButton = UIButton(type: .system)
  private let stackView = UIStackView()
  private let subtitleButton = UIButton(type: .system)

  override func viewDidLoad() {
    super.viewDidLoad()
    setupViews()
    setupLayout()
  }

  private func setupViews() {
    view.backgroundColor = .white

    titleLabel.text = "If trip navigation was worth a few dollars, please donate to support developers of this community-developed"
    titleLabel.font = UIFont.regular14()
    titleLabel.numberOfLines = 0
    titleLabel.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(titleLabel)

    closeButton.setImage(UIImage(named: "ic_nav_bar_close"), for: .normal)
    closeButton.tintColor = .black
    closeButton.translatesAutoresizingMaskIntoConstraints = false
    closeButton.addTarget(self, action: #selector(hide), for: .touchUpInside)
    view.addSubview(closeButton)

    stackView.axis = .horizontal
    stackView.alignment = .fill
    stackView.distribution = .fillEqually
    stackView.spacing = Constants.spacing
    stackView.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(stackView)

    [
      "€ 5",
      "€ 10",
      "€ 20",
      "€ 50", 
      "Other"]
      .forEach { title in
      let button = createButtonWithTitle(title)
      button.addTarget(self, action: #selector(donateButtonTapped(_:)), for: .touchUpInside)
      stackView.addArrangedSubview(button)
    }

    subtitleButton.setTitle("Already Donated", for: .normal)
    subtitleButton.backgroundColor = .clear
    subtitleButton.setTitleColor(.linkBlue(), for: .normal)
    subtitleButton.translatesAutoresizingMaskIntoConstraints = false
    subtitleButton.addTarget(self, action: #selector(hide), for: .touchUpInside)
    view.addSubview(subtitleButton)
  }

  private func setupLayout() {
    NSLayoutConstraint.activate([
      titleLabel.topAnchor.constraint(equalTo: view.topAnchor, constant: Constants.titleTopPadding),
      titleLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: Constants.titleLeadingPadding),
      titleLabel.trailingAnchor.constraint(equalTo: closeButton.leadingAnchor, constant: -Constants.titleTrailingPadding),

      closeButton.widthAnchor.constraint(equalToConstant: Constants.closeButtonSize),
      closeButton.heightAnchor.constraint(equalToConstant: Constants.closeButtonSize),
      closeButton.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: Constants.closeButtonTrailingPadding),
      closeButton.topAnchor.constraint(equalTo: view.topAnchor, constant: Constants.closeButtonTopPadding),

      stackView.topAnchor.constraint(equalTo: titleLabel.bottomAnchor, constant: Constants.stackViewTopPadding),
      stackView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: Constants.titleLeadingPadding),
      stackView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -Constants.titleLeadingPadding),

      subtitleButton.topAnchor.constraint(equalTo: stackView.bottomAnchor, constant: Constants.subtitleButtonTopPadding),
      subtitleButton.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: Constants.titleLeadingPadding),
      subtitleButton.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -Constants.titleLeadingPadding),
      subtitleButton.bottomAnchor.constraint(equalTo: view.bottomAnchor, constant: Constants.subtitleButtonBottomPadding)
    ])
  }

  private func createButtonWithTitle(_ title: String) -> UIButton {
    let button = UIButton(type: .system)
    button.setTitle(title, for: .normal)
    button.setTitleColor(.white, for: .normal)
    button.titleLabel?.font = UIFont.regular14()
    button.backgroundColor = .linkBlue()
    button.layer.setCorner(radius: Constants.cornerRadius)
    button.layer.masksToBounds = true
    return button
  }

  @objc private func hide() {
    UIView.transition(with: view, duration: kDefaultAnimationDuration / 2, options: .transitionCrossDissolve) {
      self.view.isHidden = true
    }
  }

  @objc private func donateButtonTapped(_ sender: UIButton) {
    // TODO: Link tho the donation page
    openUrl(Settings.donateUrl())
  }
}
