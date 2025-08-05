import UIKit

class BottomMenuLayersCell: UITableViewCell {
  @IBOutlet weak var closeButton: CircleImageButton!

  @IBOutlet private var subwayButton: BottomMenuLayerButton! {
    didSet {
      updateSubwayButton()
    }
  }
  @IBOutlet private var isoLinesButton: BottomMenuLayerButton! {
    didSet {
      updateIsoLinesButton()
    }
  }
  @IBOutlet private var outdoorButton: BottomMenuLayerButton! {
    didSet {
      updateOutdoorButton()
    }
  }
  @IBOutlet private var hikingButton: BottomMenuLayerButton! {
    didSet {
      updateHikingButton()
    }
  }
  @IBOutlet private var cyclingButton: BottomMenuLayerButton! {
    didSet {
      updateCyclingButton()
    }
  }

  var onClose: (()->())?
  
  override func awakeFromNib() {
    super.awakeFromNib()
    MapOverlayManager.add(self)
    closeButton.setImage(UIImage(named: "ic_close"))
    setupButtons()
  }

  private func setupButtons() {
    outdoorButton.setupWith(image: UIImage(resource: .btnMenuOutdoors), text: L("button_layer_outdoor"))
    isoLinesButton.setupWith(image: UIImage(resource: .btnMenuIsomaps), text: L("button_layer_isolines"))
    hikingButton.setupWith(image: UIImage(resource: .btnMenuHiking), text: L("button_layer_hiking"))
    cyclingButton.setupWith(image: UIImage(resource: .btnMenuCycling), text: L("button_layer_cycling"))
    subwayButton.setupWith(image: UIImage(resource: .btnMenuSubway), text: L("button_layer_subway"))
  }

  deinit {
    MapOverlayManager.remove(self)
  }
  
  override func setSelected(_ selected: Bool, animated: Bool) {
    super.setSelected(selected, animated: animated)
  }
  
  private func updateSubwayButton() {
    let enabled = MapOverlayManager.transitEnabled()
    subwayButton.setLayerEnabled(enabled)
  }
  
  private func updateIsoLinesButton() {
    let enabled = MapOverlayManager.isoLinesEnabled()
    isoLinesButton.setLayerEnabled(enabled)
  }
    
  private func updateOutdoorButton() {
    let enabled = MapOverlayManager.outdoorEnabled()
    outdoorButton.setLayerEnabled(enabled)
  }

  private func updateHikingButton() {
    let enabled = MapOverlayManager.hikingEnabled()
    hikingButton.setLayerEnabled(enabled)
  }

  private func updateCyclingButton() {
    let enabled = MapOverlayManager.cyclingEnabled()
    cyclingButton.setLayerEnabled(enabled)
  }
  
  @IBAction func onCloseButtonPressed(_ sender: Any) {
    onClose?()
  }
  
  @IBAction func onSubwayButton(_ sender: Any) {
    let enable = !MapOverlayManager.transitEnabled()
    MapOverlayManager.setTransitEnabled(enable)
  }
  
  @IBAction func onIsoLinesButton(_ sender: Any) {
    let enable = !MapOverlayManager.isoLinesEnabled()
    MapOverlayManager.setIsoLinesEnabled(enable)
  }
    
  @IBAction func onOutdoorButton(_ sender: Any) {
    let enable = !MapOverlayManager.outdoorEnabled()
    MapOverlayManager.setOutdoorEnabled(enable)
  }

  @IBAction func onHikingButton(_ sender: Any) {
    let enable = !MapOverlayManager.hikingEnabled()
    MapOverlayManager.setHikingEnabled(enable)
    if enable {
      showUpdateToastIfNeeded()
    }
  }

  @IBAction func onCyclingButton(_ sender: Any) {
    let enable = !MapOverlayManager.cyclingEnabled()
    MapOverlayManager.setCyclingEnabled(enable)
    if enable {
      showUpdateToastIfNeeded()
    }
  }

  private func showUpdateToastIfNeeded() {
    if FrameworkHelper.needUpdateForRoutes() {
      Toast.show(withText: L("routes_update_maps_text"), alignment: .top)
    }
  }
}

extension BottomMenuLayersCell: MapOverlayManagerObserver {
  func onMapOverlayUpdated() {
    updateSubwayButton()
    updateIsoLinesButton()
    updateOutdoorButton()
    updateHikingButton()
    updateCyclingButton()
  }
}

private extension BottomMenuLayerButton {
  func setupWith(image: UIImage, text: String) {
    self.image = image
    spacing = 10
    numberOfLines = 2
    localizedText = text
  }
}
