import Combine

protocol ReviewsRepository {
  var reviewsResource: PassthroughSubject<[Review], ResourceError> { get }
  func observeReviewsForPlace(id: Int64)
  
  var isThereReviewPlannedToPublishResource: PassthroughSubject<Bool, Never> { get }
  func isThereReviewPlannedToPublish(for placeId: Int64)
  
  func postReview(review: ReviewToPost) -> AnyPublisher<SimpleResponse, ResourceError>
  
  func deleteReview(id: Int64) -> AnyPublisher<SimpleResponse, ResourceError>
  
  func syncReviews()
}
