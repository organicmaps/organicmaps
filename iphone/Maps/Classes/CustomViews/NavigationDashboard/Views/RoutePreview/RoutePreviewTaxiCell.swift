@objc(MWMRoutePreviewTaxiCell)
final class RoutePreviewTaxiCell: UICollectionViewCell {

  @IBOutlet private weak var icon: UIImageView!
  @IBOutlet private weak var title: UILabel! {
    didSet {
      title.font = UIFont.bold14()
      title.textColor = UIColor.blackPrimaryText()
    }
  }

  @IBOutlet private weak var info: UILabel! {
    didSet {
      info.font = UIFont.regular14()
      info.textColor = UIColor.blackSecondaryText()
    }
  }

  @objc func config(type: MWMRoutePreviewTaxiCellType, title: String, eta: String, price: String, currency: String) {
    let iconImage = { () -> UIImage in
      switch type {
      case .taxi: return #imageLiteral(resourceName: "icTaxiTaxi")
      case .uber: return #imageLiteral(resourceName: "icTaxiUber")
      case .yandex: return #imageLiteral(resourceName: "ic_taxi_logo_yandex")
      }
    }

    let titleString = { () -> String in
      switch type {
      case .taxi: fallthrough
      case .uber: return title
      case .yandex: return L("yandex_taxi_title")
      }
    }

    let priceString = { () -> String in
      switch type {
      case .taxi: fallthrough
      case .uber: return price
      case .yandex:
        let formatter = NumberFormatter()
        formatter.numberStyle = .currency
        formatter.currencyCode = currency
        formatter.maximumFractionDigits = 0
        if let number = UInt(price), let formattedPrice = formatter.string(from: NSNumber(value: number)) {
          return formattedPrice
        } else {
          return "\(currency) \(price)"
        }
      }
    }

    let timeString = { () -> String in
      let timeValue = DateComponentsFormatter.etaString(from: TimeInterval(eta)!)!
      return String(coreFormat: L("taxi_wait"), arguments: [timeValue])
    }

    icon.image = iconImage()
    self.title.text = titleString()
    info.text = "~ \(priceString()) • \(timeString())"
  }
}
