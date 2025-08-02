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
    hikingButton.setupWith(image: UIImage(resource: .btnMenuIsomaps), text: L("button_layer_hiking"))
    cyclingButton.setupWith(image: UIImage(resource: .btnMenuIsomaps), text: L("button_layer_cycling"))
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
    subwayButton.setStyleAndApply(styleFor(enabled))
  }
  
  private func updateIsoLinesButton() {
    let enabled = MapOverlayManager.isoLinesEnabled()
    isoLinesButton.setStyleAndApply(styleFor(enabled))
  }
    
  private func updateOutdoorButton() {
    let enabled = MapOverlayManager.outdoorEnabled()
    outdoorButton.setStyleAndApply(styleFor(enabled))
  }

  private func updateHikingButton() {
    let enabled = MapOverlayManager.hikingEnabled()
    hikingButton.setStyleAndApply(styleFor(enabled))
  }

  private func updateCyclingButton() {
    let enabled = MapOverlayManager.cyclingEnabled()
    cyclingButton.setStyleAndApply(styleFor(enabled))
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

    updateHikingButton()
    /// @todo Call MWMTrafficButtonViewController applyTheme ?
  }

  @IBAction func onCyclingButton(_ sender: Any) {
    let enable = !MapOverlayManager.cyclingEnabled()
    MapOverlayManager.setCyclingEnabled(enable)

    updateCyclingButton()
    /// @todo Call MWMTrafficButtonViewController applyTheme ?
  }
}

extension BottomMenuLayersCell: MapOverlayManagerObserver {
  func onTransitStateUpdated() {
    updateSubwayButton()
  }
  
  func onIsoLinesStateUpdated() {
    updateIsoLinesButton()
  }
    
  func onOutdoorStateUpdated() {
    updateOutdoorButton()
  }
}

private extension BottomMenuLayersCell {
  func styleFor(_ enabled: Bool) -> MapStyleSheet {
    enabled ? .mapMenuButtonEnabled : .mapMenuButtonDisabled
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
