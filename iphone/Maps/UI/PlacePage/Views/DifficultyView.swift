class DifficultyView: UIView {
  private let stackView = UIStackView()
  private var views:[UIView] = []
  var difficulty: ElevationDifficulty = .easy {
    didSet {
      updateView()
    }
  }
  var colors: [UIColor] = [.gray, .green, .orange, .red]
    {
    didSet {
      updateView()
    }
  }
  var emptyColor: UIColor = UIColor.gray {
    didSet {
      updateView()
    }
  }
  
  private let bulletSize = CGSize(width: 10, height: 10)
  private let bulletSpacing: CGFloat = 5
  private let difficultyLevelCount = 3
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    initComponent()
  }
  
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    initComponent()
  }

  private func initComponent() {
    self.addSubview(stackView)
    stackView.frame = bounds
    stackView.distribution = .fillEqually
    stackView.axis = .horizontal
    stackView.spacing = bulletSpacing
    stackView.alignment = .fill

    for _ in 0..<difficultyLevelCount {
      let view = UIView()
      stackView.addArrangedSubview(view)
      view.layer.cornerRadius = bulletSize.height / 2
      views.append(view)
    }
  }
  
  private func updateView() {
    guard colors.count > difficulty.rawValue else {
      assertionFailure("No fill color")
      return
    }
    let fillColor = colors[difficulty.rawValue]
    for (idx, view) in views.enumerated() {
      if idx < difficulty.rawValue {
        view.backgroundColor = fillColor
      } else {
        view.backgroundColor = emptyColor
      }
    }
  }
}
