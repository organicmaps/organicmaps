import UIKit

enum SearchCategoryIconRenderer {
  private struct IconStyle {
    let iconName: String
    let background: Background
  }

  private struct CacheKey: Hashable {
    let categoryName: String
    let userInterfaceStyle: UIUserInterfaceStyle.RawValue
  }

  private enum Background {
    case neutral
    case food
    case tourism
    case accommodation
    case shopping
    case medical
    case transport
    case water

    var color: UIColor {
      switch self {
      case .neutral: return UIColor(0x71, 0x70, 0x65, 1)
      case .food: return UIColor(0xBB, 0x93, 0x42, 1)
      case .tourism: return UIColor(0x80, 0x59, 0x33, 1)
      case .accommodation: return UIColor(0x66, 0x4E, 0x42, 1)
      case .shopping: return UIColor(0x8C, 0x5F, 0x93, 1)
      case .medical: return UIColor(0xC1, 0x57, 0x46, 1)
      case .transport: return UIColor(0x2E, 0x89, 0xB0, 1)
      case .water: return UIColor(0x14, 0xA0, 0xC2, 1)
      }
    }
  }

  private static let frameSize: CGFloat = 28
  private static let iconInset: CGFloat = 4.5
  private static var imageCache: [CacheKey: UIImage] = [:]

  private static let styles: [String: IconStyle] = [
    "atm": IconStyle(iconName: "ic_bm_atm", background: .neutral),
    "bank": IconStyle(iconName: "ic_bm_bank", background: .neutral),
    "children": IconStyle(iconName: "ic_bm_family", background: .tourism),
    "eat": IconStyle(iconName: "ic_bm_food", background: .food),
    "entertainment": IconStyle(iconName: "ic_bm_entertainment", background: .tourism),
    "food": IconStyle(iconName: "ic_bm_supermarket", background: .shopping),
    "fuel": IconStyle(iconName: "ic_bm_gas", background: .neutral),
    "hospital": IconStyle(iconName: "ic_bm_hospital", background: .medical),
    "hotel": IconStyle(iconName: "ic_bm_hotel", background: .accommodation),
    "nightlife": IconStyle(iconName: "ic_bm_bar", background: .food),
    "parking": IconStyle(iconName: "ic_bm_parking", background: .transport),
    "pharmacy": IconStyle(iconName: "ic_bm_pharmacy", background: .medical),
    "police": IconStyle(iconName: "ic_bm_police", background: .neutral),
    "post": IconStyle(iconName: "ic_bm_mail", background: .neutral),
    "recycling": IconStyle(iconName: "ic_bm_recycling", background: .neutral),
    "rv": IconStyle(iconName: "ic_bm_caravan_site", background: .accommodation),
    "secondhand": IconStyle(iconName: "ic_bm_shop_secondhand", background: .shopping),
    "shopping": IconStyle(iconName: "ic_bm_shop", background: .shopping),
    "toilet": IconStyle(iconName: "ic_bm_toilets", background: .tourism),
    "tourism": IconStyle(iconName: "ic_bm_sights", background: .tourism),
    "transport": IconStyle(iconName: "ic_bm_transport", background: .transport),
    "water": IconStyle(iconName: "ic_bm_water", background: .water),
    "wifi": IconStyle(iconName: "ic_bm_wifi", background: .neutral),
  ]

  static func image(for categoryName: String, userInterfaceStyle: UIUserInterfaceStyle) -> UIImage? {
    let normalizedName = normalizedCategoryName(categoryName)
    guard let style = styles[normalizedName] else { return nil }

    let cacheKey = CacheKey(categoryName: normalizedName, userInterfaceStyle: userInterfaceStyle.rawValue)
    if let image = imageCache[cacheKey] {
      return image
    }

    guard UIImage(named: style.iconName) != nil else { return nil }

    let backgroundColor = style.background.color
    let iconColor: UIColor = userInterfaceStyle == .dark ? .black : .white
    let image = circleImageForColor(backgroundColor,
                                    frameSize: frameSize,
                                    iconName: style.iconName,
                                    iconInset: iconInset,
                                    iconColor: iconColor).withRenderingMode(.alwaysOriginal)
    imageCache[cacheKey] = image
    return image
  }

  private static func normalizedCategoryName(_ categoryName: String) -> String {
    let prefix = "category_"
    guard categoryName.hasPrefix(prefix) else { return categoryName }
    return String(categoryName.dropFirst(prefix.count))
  }
}
