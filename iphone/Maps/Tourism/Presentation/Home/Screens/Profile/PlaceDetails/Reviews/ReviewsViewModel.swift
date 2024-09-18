import Combine

class ReviewsViewModel: ObservableObject {
  private var cancellables = Set<AnyCancellable>()
  
  private let reviewsRepository: ReviewsRepository
  private let userPreferences: UserPreferences
  
  private let placeId: Int64
  
  @Published var messageToShowOnReviewsScreen = ""
  @Published var shouldShowMessageOnReviewsScreen = false
  
  @Published var reviews: [Review] = []
  @Published var userReview: Review? = nil
  @Published var latestReview: Review? = nil
  @Published var isThereReviewPlannedToPublish = false
  
  init(reviewsRepository: ReviewsRepository, userPreferences: UserPreferences, id: Int64) {
    self.reviewsRepository = reviewsRepository
    self.userPreferences = userPreferences
    self.placeId = id
    
    observeReviews()
    observeIfThereIsReviewPlannedToPost()
  }
  
  func observeReviews() {
    reviewsRepository.observeReviewsForPlace(id: placeId)
    
    reviewsRepository.reviewsResource.sink { _ in } receiveValue: { reviews in
      self.reviews = reviews
      
      self.getUserReview()
      self.getLatestReview()
    }
    .store(in: &cancellables)
  }
  
  private func getUserReview() {
    let userId = userPreferences.getUserId()
    let first = reviews.filter {
      if let user = $0.user {
        return String(user.id) == userId
      } else {
        return false
      }
    }.first
    
    if let userReview = first {
      self.userReview = userReview
    }
  }
  
  private func getLatestReview() {
    if let latest = reviews.first {
      self.latestReview = latest
    }
  }
  
  private func observeIfThereIsReviewPlannedToPost() {
    reviewsRepository.checkIfThereIsReviewPlannedToPublish(for: placeId)
    
    reviewsRepository.isThereReviewPlannedToPublishResource.sink { _ in } receiveValue: { isThere in
      self.isThereReviewPlannedToPublish = isThere
    }
    .store(in: &cancellables)
  }
  
  func deleteReview() {
    if let id = userReview?.id {
      reviewsRepository.deleteReview(id: id)
        .sink(receiveCompletion: { completion in
          switch completion {
          case .finished:
            self.userReview = nil
          case .failure(let error):
            self.showMessageOnReviewsScreen(error.localizedDescription)
          }
        }, receiveValue: { response in
          self.showMessageOnReviewsScreen(response.message)
        })
        .store(in: &cancellables)
    }
  }
  
  func showMessageOnReviewsScreen(_ message: String) {
    messageToShowOnReviewsScreen = message
    shouldShowMessageOnReviewsScreen = true
  }
}
