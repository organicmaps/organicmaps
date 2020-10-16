@objc(MWMRoutePreviewTaxiCell)
final class RoutePreviewTaxiCell: UICollectionViewCell {

  @IBOutlet private weak var icon: UIImageView!
  @IBOutlet private weak var title: UILabel!
  @IBOutlet private weak var info: UILabel!

  @objc func config(type: MWMRoutePreviewTaxiCellType, title: String, eta: String, price: String, currency: String) {
    let iconImage = { () -> UIImage in
      switch type {
      case .taxi: return #imageLiteral(resourceName: "icTaxiTaxi")
      case .uber: return #imageLiteral(resourceName: "icTaxiUber")
      case .yandex: return #imageLiteral(resourceName: "ic_taxi_logo_yandex")
      case .maxim: return #imageLiteral(resourceName: "ic_taxi_logo_maksim")
      case .vezet: return #imageLiteral(resourceName: "ic_taxi_logo_vezet")
      case .freenow: return #imageLiteral(resourceName: "ic_logo_freenow")
      case .yango: return #imageLiteral(resourceName: "ic_taxi_logo_yango")
      case .citymobil: return #imageLiteral(resourceName: "ic_taxi_logo_citymobil_light")
      }
    }

    let titleString = { () -> String in
      switch type {
      case .taxi, .uber, .freenow, .citymobil: return title
      case .yandex: return L("yandex_taxi_title")
      case .maxim: return L("maxim_taxi_title")
      case .vezet: return L("vezet_taxi")
      case .yango: return L("yango_taxi_title")
      }
    }

    let priceString = { () -> String in
      switch type {
      case .taxi, .uber, .freenow: return price
      case .yandex, .maxim, .vezet, .yango, .citymobil:
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
      var timeValue = DateComponentsFormatter.etaString(from: TimeInterval(eta)!)!
      
      if type == .vezet {
        timeValue = String(coreFormat: L("place_page_starting_from"), arguments: [timeValue]);
      }
      
      return String(coreFormat: L("taxi_wait"), arguments: [timeValue])
    }

    icon.image = iconImage()
    self.title.text = titleString()
    info.text = "~ \(priceString()) • \(timeString())"
  }
}
