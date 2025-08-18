class ActivityIndicator: UIView {
  let spinner = UIImageView(image: UIImage(named: "ic_24px_spinner"))

  override var intrinsicContentSize: CGSize {
    return CGSize(width: 24, height: 24)
  }

  init() {
    super.init(frame: CGRect(x: 0, y: 0, width: 24, height: 24))
    setup()
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    setup()
  }
  
  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setup()
  }

  private func setup() {
    addSubview(spinner)
    NSLayoutConstraint.activate([
      spinner.topAnchor.constraint(equalTo: topAnchor),
      spinner.leftAnchor.constraint(equalTo: leftAnchor),
      spinner.bottomAnchor.constraint(equalTo: bottomAnchor),
      spinner.rightAnchor.constraint(equalTo: rightAnchor)
      ])
    startRotation()
  }
}
