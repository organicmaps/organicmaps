import UIKit

class BottomMenuLayersCell: UITableViewCell {
  @IBOutlet var guidesButton: VerticallyAlignedButton! {
    didSet {
      updateGuidesButton()
    }
  }
  @IBOutlet private var trafficButton: VerticallyAlignedButton! {
    didSet {
      updateTrafficButton()
    }
  }
  @IBOutlet private var subwayButton: VerticallyAlignedButton! {
    didSet {
      updateSubwayButton()
    }
  }
  @IBOutlet private var isoLinesButton: VerticallyAlignedButton! {
    didSet {
      updateIsoLinesButton()
    }
  }
  
  var onClose: (()->())?
  
  override func awakeFromNib() {
    super.awakeFromNib()
    MapOverlayManager.add(self)
  }
  
  deinit {
    MapOverlayManager.remove(self)
  }
  
  override func setSelected(_ selected: Bool, animated: Bool) {
    super.setSelected(selected, animated: animated)
  }
  
  private func updateGuidesButton() {
    let enabled = MapOverlayManager.guidesEnabled()
    let isFirtLaunch = MapOverlayManager.guidesFirstLaunch()
    if enabled {
      guidesButton.setStyleAndApply("LayersGuidesButtonEnabled")
    } else {
      guidesButton.setStyleAndApply(isFirtLaunch ? "LayersGuidesButtonFirstLaunch" : "LayersGuidesButtonDisabled")
    }
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
  
  @IBAction func onCloseButtonPressed(_ sender: Any) {
    onClose?()
  }
  
  @IBAction func onGuidesButtonPressed(_ sender: Any) {
    MapOverlayManager.setGuidesEnabled(!MapOverlayManager.guidesEnabled())
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

extension BottomMenuLayersCell: MapOverlayManagerObserver {
  func onGuidesStateUpdated() {
    updateGuidesButton()
    let status: String?
    switch MapOverlayManager.guidesState() {
    case .enabled:
      status = "success"
    case .noData:
      status = "unavailable"
    case .networkError:
      status = "error"
    case .disabled:
      status = nil
    @unknown default:
      fatalError()
    }
    
    if let status = status {
      Statistics.logEvent("Map_Layers_activate", withParameters: ["name" : "guides",
                                                                  "status" : status])
    }
  }
  
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
