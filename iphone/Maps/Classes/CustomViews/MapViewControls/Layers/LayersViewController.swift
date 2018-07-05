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
    MWMTrafficManager.enableTraffic(!MWMTrafficManager.trafficEnabled())
  }

  @IBAction func onSubwayButton(_ sender: UIButton) {
    MWMTrafficManager.enableTransit(!MWMTrafficManager.transitEnabled())
  }
}
