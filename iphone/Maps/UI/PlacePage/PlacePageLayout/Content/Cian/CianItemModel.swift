@objc(MWMCianItemModel)
final class CianItemModel: NSObject {
  let roomsCount: UInt
  let priceRur: UInt
  let pageURL: URL
  let address: String

  @objc init(roomsCount: UInt, priceRur: UInt, pageURL: URL, address: String) {
    self.roomsCount = roomsCount
    self.priceRur = priceRur
    self.pageURL = pageURL
    self.address = address
  }
}
