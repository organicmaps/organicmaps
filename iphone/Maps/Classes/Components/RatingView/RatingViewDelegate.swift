import Foundation

@objc
protocol RatingViewDelegate: class {
  func didTouchRatingView(_ view: RatingView)
  func didFinishTouchingRatingView(_ view: RatingView)
}
