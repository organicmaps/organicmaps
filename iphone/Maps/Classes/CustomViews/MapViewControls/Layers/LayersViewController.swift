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
    let status: String?
    switch MWMTrafficManager.trafficState() {
    case .enabled: status = "success"
    case .noData: status = "unavailable"
    case .networkError: status = "error"
    case .disabled: fallthrough
    case .waitingData: fallthrough
    case .outdated: fallthrough
    case .expiredData: fallthrough
    case .expiredApp: status = nil
    }

    if let status = status {
      Statistics.logEvent("Map_Layers_activate", withParameters: ["name" : "traffic",
                                                                  "status" : status])
    }
  }

  func onTransitStateUpdated() {
    updateSubwayButton()
    let status: String?
    switch MWMTrafficManager.transitState() {
    case .enabled: status = "success"
    case .noData: status = "unavailable"
    case .disabled: status = nil
    }

    if let status = status {
      Statistics.logEvent("Map_Layers_activate", withParameters: ["name" : "subway",
                                                                  "status" : status])
    }
  }
}
