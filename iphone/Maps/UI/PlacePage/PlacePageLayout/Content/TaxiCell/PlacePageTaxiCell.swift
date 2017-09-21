@objc(MWMPlacePageTaxiCell)
final class PlacePageTaxiCell: MWMTableViewCell {
  @IBOutlet private weak var icon: UIImageView!
  @IBOutlet private weak var title: UILabel! {
    didSet {
      title.font = UIFont.bold14()
      title.textColor = UIColor.blackPrimaryText()
    }
  }

  @IBOutlet private weak var orderButton: UIButton! {
    didSet {
      let l = orderButton.layer
      l.cornerRadius = 6
      l.borderColor = UIColor.linkBlue().cgColor
      l.borderWidth = 1
      orderButton.setTitle(L("taxi_order"), for: .normal)
      orderButton.setTitleColor(UIColor.white, for: .normal)
      orderButton.titleLabel?.font = UIFont.bold14()
      orderButton.backgroundColor = UIColor.linkBlue()
    }
  }

  private weak var delegate: MWMPlacePageButtonsProtocol!
  private var type: MWMPlacePageTaxiProvider!

  @objc func config(type: MWMPlacePageTaxiProvider, delegate: MWMPlacePageButtonsProtocol) {
    self.delegate = delegate
    self.type = type
    switch type {
    case .taxi:
      icon.image = #imageLiteral(resourceName: "icTaxiTaxi")
      title.text = L("taxi")
    case .uber:
      icon.image = #imageLiteral(resourceName: "icTaxiUber")
      title.text = L("uber")
    case .yandex:
      icon.image = #imageLiteral(resourceName: "icTaxiYandex")
      title.text = L("yandex_taxi_title")
    }
  }

  @IBAction func orderAction() {
    delegate.orderTaxi(type)
  }
}
