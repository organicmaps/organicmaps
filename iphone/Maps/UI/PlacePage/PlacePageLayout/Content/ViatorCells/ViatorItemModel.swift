@objc(MWMViatorItemModel)
final class ViatorItemModel: NSObject {
  let imageURL: URL
  let pageURL: URL
  let title: String
  let ratingFormatted: String
  let ratingType: MWMRatingSummaryViewValueType
  let duration: String
  let price: String

  @objc init(imageURL: URL, pageURL: URL, title: String, ratingFormatted: String, ratingType: MWMRatingSummaryViewValueType, duration: String, price: String) {
    self.imageURL = imageURL
    self.pageURL = pageURL
    self.title = title
    self.ratingFormatted = ratingFormatted
    self.ratingType = ratingType
    self.duration = duration
    self.price = price
  }
}
