final class LayersViewController: MWMViewController {
  private let transitioning = CoverVerticalModalTransitioning(presentationHeight: 150)

  @IBOutlet var trafficButton: VerticallyAlignedButton! {
    didSet {
      updateTrafficButton()
    }
  }
  @IBOutlet var subwayButton: VerticallyAlignedButton! {
    didSet {
      updateSubwayButton()
    }
  }
  @IBOutlet var isoLinesButton: VerticallyAlignedButton! {
    didSet {
      updateIsoLinesButton()
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    MapOverlayManager.add(self)
  }

  deinit {
    MapOverlayManager.remove(self)
  }

  override var transitioningDelegate: UIViewControllerTransitioningDelegate? {
    get { return transitioning }
    set { }
  }

  override var modalPresentationStyle: UIModalPresentationStyle {
    get { return .custom }
    set { }
  }

  private func updateTrafficButton() {
    let enabled = MapOverlayManager.trafficEnabled()
    trafficButton.setTitleColor(enabled ? UIColor.linkBlue() : UIColor.blackSecondaryText(), for: .normal)
    trafficButton.imageName = enabled ? "btn_menu_traffic_on" : "btn_menu_traffic_off"
  }

  private func updateSubwayButton() {
    let enabled = MapOverlayManager.transitEnabled()
    subwayButton.setTitleColor(enabled ? UIColor.linkBlue() : UIColor.blackSecondaryText(), for: .normal)
    subwayButton.imageName = enabled ? "btn_menu_subway_on" : "btn_menu_subway_off"
  }

  private func updateIsoLinesButton() {
    let enabled = MapOverlayManager.isoLinesEnabled()
    isoLinesButton.setTitleColor(enabled ? UIColor.linkBlue() : UIColor.blackSecondaryText(), for: .normal)
    isoLinesButton.imageName = enabled ? "btn_menu_isomaps_on" : "btn_menu_isomaps_off"
  }

  @IBAction func onTrafficButton(_ sender: UIButton) {
    MapOverlayManager.setTrafficEnabled(!MapOverlayManager.trafficEnabled())
  }

  @IBAction func onSubwayButton(_ sender: UIButton) {
    MapOverlayManager.setTransitEnabled(!MapOverlayManager.transitEnabled())
  }

  @IBAction func onIsoLinesButton(_ sender: UIButton) {
    MapOverlayManager.setIsoLinesEnabled(!MapOverlayManager.isoLinesEnabled())
  }
}

extension LayersViewController: MapOverlayManagerObserver {
  func onTrafficStateUpdated() {
    updateTrafficButton()
    let status: String?
    switch MapOverlayManager.trafficState() {
    case .enabled:
      status = "success"
    case .noData:
      status = "unavailable"
    case .networkError:
      status = "error"
    case .disabled, .waitingData, .outdated, .expiredData, .expiredApp:
      status = nil
    @unknown default:
      fatalError()
    }

    if let status = status {
      Statistics.logEvent("Map_Layers_activate", withParameters: ["name" : "traffic",
                                                                  "status" : status])
    }
  }

  func onTransitStateUpdated() {
    updateSubwayButton()
    let status: String?
    switch MapOverlayManager.transitState() {
    case .enabled:
      status = "success"
    case .noData:
      status = "unavailable"
    case .disabled:
      status = nil
    @unknown default:
      fatalError()
    }

    if let status = status {
      Statistics.logEvent("Map_Layers_activate", withParameters: ["name" : "subway",
                                                                  "status" : status])
    }
  }

  func onIsoLinesStateUpdated() {
    updateIsoLinesButton()
    if MapOverlayManager.isoLinesEnabled() {
      Statistics.logEvent("Map_Layers_activate", withParameters: ["name" : "isolines",
                                                                  "status" : "success"])
    }
  }
}
