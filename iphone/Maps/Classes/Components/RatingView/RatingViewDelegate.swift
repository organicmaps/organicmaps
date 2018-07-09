import Foundation

@objc
protocol RatingViewDelegate: AnyObject {
  func didTouchRatingView(_ view: RatingView)
  func didFinishTouchingRatingView(_ view: RatingView)
}
