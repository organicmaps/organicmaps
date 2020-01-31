class HotelFacilityViewController: UIViewController {
  @IBOutlet var facilityLabel: UILabel!
  var facility: String!

  override func viewDidLoad() {
    super.viewDidLoad()
    facilityLabel.text = facility
  }
}

protocol HotelFacilitiesViewControllerDelegate: AnyObject {
  func facilitiesDidPressMore()
}

class HotelFacilitiesViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!

  var facilities: [HotelFacility]? {
    didSet {
      updateFacilities()
    }
  }
  weak var delegate: HotelFacilitiesViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()
    updateFacilities()
  }

  private func updateFacilities() {
    guard let facilities = facilities else { return }

    let count = min(facilities.count, 3)
    for i in 0..<count {
      addFacility(facilities[i].name)
    }

    if facilities.count > count {
      addMoreButton()
    }
  }

  @objc func onMoreButton(_ sender: UIButton) {
    delegate?.facilitiesDidPressMore()
  }

  private func addFacility(_ facility: String) {
    let vc = storyboard!.instantiateViewController(ofType: HotelFacilityViewController.self)
    vc.facility = facility
    addChild(vc)
    stackView.addArrangedSubview(vc.view)
    vc.didMove(toParent: self)
  }

  private func addMoreButton() {
    let button = UIButton()
    button.setTitle(L("booking_show_more"), for: .normal)
    button.styleName = "MoreButton"
    button.heightAnchor.constraint(equalToConstant: 44).isActive = true
    stackView.addArrangedSubview(button)
    button.addTarget(self, action: #selector(onMoreButton(_:)), for: .touchUpInside)
  }
}
