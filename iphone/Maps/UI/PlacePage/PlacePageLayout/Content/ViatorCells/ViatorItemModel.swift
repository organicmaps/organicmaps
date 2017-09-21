@objc(MWMViatorItemModel)
final class ViatorItemModel: NSObject {
  let imageURL: URL
  let pageURL: URL
  let title: String
  let rating: Double
  let duration: String
  let price: String

  @objc init(imageURL: URL, pageURL: URL, title: String, rating: Double, duration: String, price: String) {
    self.imageURL = imageURL
    self.pageURL = pageURL
    self.title = title
    self.rating = rating
    self.duration = duration
    self.price = price
  }
}
