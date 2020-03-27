protocol ElevationDetailsViewProtocol: class {
  var presenter: ElevationDetailsPresenterProtocol?  { get set }
  
  func setExtendedDifficultyGrade (_ value: String)
  func setDifficulty(_ value: ElevationDifficulty)
  func setDifficultyDescription(_ value: String)
}

class ElevationDetailsViewController: MWMViewController {
  private let transitioning = FadeTransitioning<AlertPresentationController>()
  var presenter: ElevationDetailsPresenterProtocol?
  @IBOutlet var headerTitle: UILabel!
  @IBOutlet var difficultyView: DifficultyView!
  @IBOutlet var difficultyLabel: UILabel!
  @IBOutlet private var extendedDifficultyGradeLabel: UILabel!
  @IBOutlet var difficultyDescriptionLabel: UILabel!
  
  override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
    transitioningDelegate = transitioning
    modalPresentationStyle = .custom
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()
  }
  
  @IBAction func onOkButtonPressed(_ sender: Any) {
    presenter?.onOkButtonPressed()
  }
}

extension ElevationDetailsViewController: ElevationDetailsViewProtocol {
  func setExtendedDifficultyGrade (_ value: String) {
    extendedDifficultyGradeLabel.text = value
  }
  
  func setDifficulty(_ value: ElevationDifficulty) {
    difficultyView.difficulty = value
    switch value {
    case .easy:
      difficultyLabel.text = L("elevation_profile_diff_level_easy")
    case .medium:
      difficultyLabel.text = L("elevation_profile_diff_level_moderate")
    case .hard:
      difficultyLabel.text = L("elevation_profile_diff_level_hard")
    default:
      break;
    }
  }
  
  func setDifficultyDescription(_ value: String) {
    difficultyDescriptionLabel.text = value
  }
}
