@objc enum SearchBarState: Int {
  case ready
  case searching
  case back
}

final class SearchBar: SolidTouchView {
  @IBOutlet private var searchIcon: UIImageView!
  @IBOutlet private var activityIndicator: UIActivityIndicatorView!
  @IBOutlet private var backButton: UIButton!
  @IBOutlet private var searchTextField: SearchTextField!
  @IBOutlet private var stackView: UIStackView!

  override var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: .top, iPad: .left) }

  override var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: [], iPad: .left) }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: [], iPad: .left) }

  override var trafficButtonAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: .top, iPad: .left) }

  override var tabBarAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: [], iPad: .left) }

  @objc var state: SearchBarState = .ready {
    didSet {
      if state != oldValue {
        updateLeftView()
      }
    }
  }

  private lazy var dateFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.setLocalizedDateFormatFromTemplate("EEE, MMMd")
    return formatter
  }()

  override func awakeFromNib() {
    super.awakeFromNib()
    updateLeftView()
    searchTextField.leftViewMode = UITextField.ViewMode.always
    searchTextField.leftView = UIView(frame: CGRect(x: 0, y: 0, width: 32, height: 32))
  }

  private func updateLeftView() {
    searchIcon.isHidden = true
    activityIndicator.isHidden = true
    backButton.isHidden = true

    switch state {
    case .ready:
      searchIcon.isHidden = false
    case .searching:
      activityIndicator.isHidden = false
      activityIndicator.startAnimating()
    case .back:
      backButton.isHidden = false
    }
  }
}
