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
    trafficButton.setStyleAndApply(enabled ? "LayersTrafficButtonEnabled" : "LayersTrafficButtonDisabled")
  }

  private func updateSubwayButton() {
    let enabled = MapOverlayManager.transitEnabled()
    subwayButton.setStyleAndApply(enabled ? "LayersSubwayButtonEnabled" : "LayersSubwayButtonDisabled")
  }

  private func updateIsoLinesButton() {
    let enabled = MapOverlayManager.isoLinesEnabled()
    isoLinesButton.setStyleAndApply(enabled ? "LayersIsolinesButtonEnabled" : "LayersIsolinesButtonDisabled")
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
    let status: String?
    switch MapOverlayManager.isolinesState() {
    case .enabled:
      status = "success"
    case .expiredData:
      status = "error"
    case .noData:
      status = "unavailable"
    case .disabled:
      status = nil
    @unknown default:
      fatalError()
    }
    
    if let status = status {
      Statistics.logEvent("Map_Layers_activate", withParameters: ["name" : "isolines",
                                                                  "status" : status])
    }
  }
}
