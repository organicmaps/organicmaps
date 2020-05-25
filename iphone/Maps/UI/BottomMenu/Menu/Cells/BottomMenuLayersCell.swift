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
    let enable = !MapOverlayManager.guidesEnabled()
    MapOverlayManager.setGuidesEnabled(enable)
    Statistics.logEvent(kStatLayersClick, withParameters: [kStatName : kStatGuides,
                                                           kStatFrom : kStatMenu,
                                                           kStatTurnOn : enable])
  }
  @IBAction func onTrafficButton(_ sender: UIButton) {
    let enable = !MapOverlayManager.trafficEnabled()
    MapOverlayManager.setTrafficEnabled(enable)
    Statistics.logEvent(kStatLayersClick, withParameters: [kStatName : kStatTraffic,
                                                           kStatFrom : kStatMenu,
                                                           kStatTurnOn : enable])
  }
  
  @IBAction func onSubwayButton(_ sender: UIButton) {
    let enable = !MapOverlayManager.transitEnabled()
    MapOverlayManager.setTransitEnabled(enable)
    Statistics.logEvent(kStatLayersClick, withParameters: [kStatName : kStatSubway,
                                                           kStatFrom : kStatMenu,
                                                           kStatTurnOn : enable])
  }
  
  @IBAction func onIsoLinesButton(_ sender: UIButton) {
    let enable = !MapOverlayManager.isoLinesEnabled()
    MapOverlayManager.setIsoLinesEnabled(enable)
    Statistics.logEvent(kStatLayersClick, withParameters: [kStatName : kStatIsolines,
                                                           kStatFrom : kStatMenu,
                                                           kStatTurnOn : enable])
  }
}

extension BottomMenuLayersCell: MapOverlayManagerObserver {
  func onGuidesStateUpdated() {
    updateGuidesButton()
  }
  
  func onTrafficStateUpdated() {
    updateTrafficButton()
  }
  
  func onTransitStateUpdated() {
    updateSubwayButton()
  }
  
  func onIsoLinesStateUpdated() {
    updateIsoLinesButton()
  }
}
