protocol BottomMenuViewProtocol: class {
  var presenter: BottomMenuPresenterProtocol?  { get set }
}

class BottomMenuViewController: MWMViewController {
  var presenter: BottomMenuPresenterProtocol?
  private let transitioningManager = BottomMenuTransitioningManager()
  
  @IBOutlet var tableView: UITableView!
  @IBOutlet var heightConstraint: NSLayoutConstraint!
  
  lazy var chromeView: UIView = {
    let view = UIView()
    view.styleName = "BlackStatusBarBackground"
    return view
  }()
  
  weak var containerView: UIView! {
    didSet {
      containerView.insertSubview(chromeView, at: 0)
    }
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    tableView.layer.cornerRadius = 8
    tableView.layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    
    tableView.dataSource = presenter
    tableView.delegate = presenter
    tableView.registerNib(cell: BottomMenuItemCell.self)
    tableView.registerNib(cell: BottomMenuLayersCell.self)
  }
  
  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    heightConstraint.constant = min(self.tableView.contentSize.height, self.view.height)
  }
  
  @IBAction func onClosePressed(_ sender: Any) {
    presenter?.onClosePressed()
  }
  
  override var transitioningDelegate: UIViewControllerTransitioningDelegate? {
    get { return transitioningManager }
    set { }
  }
  
  override var modalPresentationStyle: UIModalPresentationStyle {
    get { return .custom }
    set { }
  }
}

extension BottomMenuViewController: BottomMenuViewProtocol {
  
}
