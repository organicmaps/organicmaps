@objc(MWMCianItemModel)
final class CianItemModel: NSObject {
  let roomsCount: UInt
  let priceRur: UInt
  let pageURL: URL
  let address: String

  init(roomsCount: UInt, priceRur: UInt, pageURL: URL, address: String) {
    self.roomsCount = roomsCount
    self.priceRur = priceRur
    self.pageURL = pageURL
    self.address = address
  }
}
