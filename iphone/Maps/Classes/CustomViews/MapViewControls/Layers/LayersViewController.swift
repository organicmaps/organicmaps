final class LayersViewController: MWMViewController {

  @IBOutlet weak var trafficButton: VerticallyAlignedButton! {
    didSet {
      updateTrafficButton()
    }
  }
  @IBOutlet weak var subwayButton: VerticallyAlignedButton! {
    didSet {
      updateSubwayButton()
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    MWMTrafficManager.add(self)
  }

  deinit {
    MWMTrafficManager.remove(self)
  }

  override var transitioningDelegate: UIViewControllerTransitioningDelegate? {
    get { return ModalTransitioning() }
    set { }
  }

  private func updateTrafficButton() {
    let enabled = MWMTrafficManager.trafficEnabled()
    trafficButton.setTitleColor(enabled ? UIColor.linkBlue() : UIColor.blackSecondaryText(), for: .normal)
    trafficButton.imageName = enabled ? "btn_menu_traffic_on" : "btn_menu_traffic_off"
  }

  private func updateSubwayButton() {
    let enabled = MWMTrafficManager.transitEnabled()
    subwayButton.setTitleColor(enabled ? UIColor.linkBlue() : UIColor.blackSecondaryText(), for: .normal)
    subwayButton.imageName = enabled ? "btn_menu_subway_on" : "btn_menu_subway_off"
  }

  @IBAction func onTrafficButton(_ sender: UIButton) {
    MWMTrafficManager.setTrafficEnabled(!MWMTrafficManager.trafficEnabled())
  }

  @IBAction func onSubwayButton(_ sender: UIButton) {
    MWMTrafficManager.setTransitEnabled(!MWMTrafficManager.transitEnabled())
  }
}

extension LayersViewController: MWMTrafficManagerObserver {
  func onTrafficStateUpdated() {
    updateTrafficButton()
    let statusString: String
    switch MWMTrafficManager.trafficState() {
    case .enabled: statusString = "success"
    case .noData: statusString = "unavailable"
    case .networkError: statusString = "error"
    case .disabled: fallthrough
    case .waitingData: fallthrough
    case .outdated: fallthrough
    case .expiredData: fallthrough
    case .expiredApp: statusString = ""
    }

    Statistics.logEvent("Map_Layers_activate", withParameters: ["name" : "traffic",
                                                                "status" : statusString])
  }

  func onTransitStateUpdated() {
    updateSubwayButton()
    let statusString: String
    switch MWMTrafficManager.transitState() {
    case .enabled: statusString = "success"
    case .noData: statusString = "unavailable"
    case .disabled: statusString = ""
    }
    
    Statistics.logEvent("Map_Layers_activate", withParameters: ["name" : "subway",
                                                                "status" : statusString])
  }
}
