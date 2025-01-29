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
