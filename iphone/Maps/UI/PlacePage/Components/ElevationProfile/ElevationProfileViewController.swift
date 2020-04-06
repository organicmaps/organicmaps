import Chart

protocol ElevationProfileViewProtocol: class {
  var presenter: ElevationProfilePresenterProtocol?  { get set }
  
  var isExtendedDifficultyLabelHidden: Bool { get set }
  var isDifficultyHidden: Bool { get set }
  var isTimeHidden: Bool { get set }
  var isBottomPanelHidden: Bool { get set }
  func setExtendedDifficultyGrade(_ value: String)
  func setTrackTime(_ value: String?)
  func setDifficulty(_ value: ElevationDifficulty)
  func setChartData(_ data: ChartPresentationData)
  func setActivePoint(_ distance: Double)
  func setMyPosition(_ distance: Double)
}

class ElevationProfileViewController: UIViewController {
  var presenter: ElevationProfilePresenterProtocol?
  
  @IBOutlet private var chartView: ChartView!
  @IBOutlet private var graphViewContainer: UIView!
  @IBOutlet private var descriptionCollectionView: UICollectionView!
  @IBOutlet private var difficultyView: DifficultyView!
  @IBOutlet private var difficultyTitle: UILabel!
  @IBOutlet private var extendedDifficultyGradeLabel: UILabel!
  @IBOutlet private var trackTimeLabel: UILabel!
  @IBOutlet private var trackTimeTitle: UILabel!
  @IBOutlet private var extendedGradeButton: UIButton!
  @IBOutlet private var diffucultyConstraint: NSLayoutConstraint!

  private let diffucultiVisibleConstraint: CGFloat = 60
  private let diffucultyHiddenConstraint: CGFloat = 10
  private var difficultyHidden: Bool = false
  private var timeHidden: Bool = false
  private var bottomPanelHidden: Bool = false

  override func viewDidLoad() {
    super.viewDidLoad()
    descriptionCollectionView.dataSource = presenter
    descriptionCollectionView.delegate = presenter
    presenter?.configure()
    chartView.onSelectedPointChanged = { [weak self] in
      self?.presenter?.onSelectedPointChanged($0)
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    presenter?.onAppear()
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    presenter?.onDissapear()
  }

  override func viewWillLayoutSubviews() {
    super.viewWillLayoutSubviews()
    descriptionCollectionView.reloadData()
  }

  @IBAction func onExtendedDifficultyButtonPressed(_ sender: Any) {
    presenter?.onDifficultyButtonPressed()
  }

  func getPreviewHeight() -> CGFloat {
    return view.height - descriptionCollectionView.frame.minY
  }
}

extension ElevationProfileViewController: ElevationProfileViewProtocol {
  var isExtendedDifficultyLabelHidden: Bool {
    get { return extendedDifficultyGradeLabel.isHidden }
    set {
      extendedDifficultyGradeLabel.isHidden = newValue
      extendedGradeButton.isHidden = newValue
    }
  }

  var isDifficultyHidden: Bool {
    get { difficultyHidden }
    set {
      difficultyHidden = newValue
      difficultyTitle.isHidden = newValue
      difficultyView.isHidden = newValue
    }
  }

  var isTimeHidden: Bool {
    get { timeHidden }
    set {
      timeHidden = newValue
      trackTimeLabel.isHidden = newValue
      trackTimeTitle.isHidden = newValue
    }
  }

  var isBottomPanelHidden: Bool {
    get { bottomPanelHidden }
    set {
      bottomPanelHidden = newValue
      if newValue == true {
        isTimeHidden = true
        isExtendedDifficultyLabelHidden = true
        isDifficultyHidden = true
      }
      diffucultyConstraint.constant = newValue ? diffucultyHiddenConstraint : diffucultiVisibleConstraint
    }
  }
  
  func setExtendedDifficultyGrade(_ value: String) {
    extendedDifficultyGradeLabel.text = value
  }

  func setTrackTime(_ value: String?) {
    trackTimeLabel.text = value
  }

  func setDifficulty(_ value: ElevationDifficulty) {
    difficultyView.difficulty = value
  }

  func setChartData(_ data: ChartPresentationData) {
    chartView.chartData = data
  }

  func setActivePoint(_ distance: Double) {
    chartView.setSelectedPoint(distance)
  }

  func setMyPosition(_ distance: Double) {
    chartView.myPosition = distance
  }
}
