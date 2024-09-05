import Combine

protocol ReviewsRepository {
  var reviewsResource: PassthroughSubject<[Review], ResourceError> { get }
  func getReviewsForPlace(id: Int64)
  
  var isThereReviewPlannedToPublishPassthroughSubject: PassthroughSubject<[Review], ResourceError> { get }
  func isThereReviewPlannedToPublish(id: Int64)
  
  func postReview(review: ReviewToPost) -> AnyPublisher<SimpleResponse, ResourceError>
  
  func deleteReview(id: Int64) -> AnyPublisher<SimpleResponse, ResourceError>
  
  func syncReviews()
}
