import Combine
import Foundation

class ReviewsPersistenceControllerTesterBro {
  private static var cancellables = Set<AnyCancellable>()
  private static let persistenceController = ReviewsPersistenceController.shared
  
  static func testAllReviews() {
    testCRUDOperations()
    testObserveReviewsForPlace()
    testMarkReviewForDeletion()
    testGetReviewsPlannedForDeletion()
  }
  
  private static func testCRUDOperations() {
    print("Testing CRUD Operations...")
    
    // Example Review objects
    let review1 = Review(
      id: 1,
      placeId: 1,
      rating: 5,
      user: User(id: 1, name: "John Doe", pfpUrl: Constants.imageUrlExample, countryCodeName: "us"),
      date: "01-01-2000",
      comment: "Great place!",
      picsUrls: [Constants.imageUrlExample,Constants.imageUrlExample,Constants.anotherImageExample],
      deletionPlanned: false
    )
    
    let review2 = Review(
      id: 2,
      placeId: 1,
      rating: 4,
      user: User(id: 1, name: "John Doe", pfpUrl: Constants.anotherImageExample, countryCodeName: "us"),
      date: "01-01-2000",
      comment: "Nice atmosphere",
      picsUrls: [Constants.imageUrlExample],
      deletionPlanned: false
    )
    
    let review3 = Review(
      id: 3,
      placeId: 1,
      rating: 4,
      user: User(id: 1, name: "John Doe", pfpUrl: Constants.anotherImageExample, countryCodeName: "us"),
      date: "01-01-2000",
      comment: "Nice atmosphere",
      picsUrls: [Constants.imageUrlExample],
      deletionPlanned: false
    )
    
    // Insert reviews
    DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
      self.persistenceController.putReview(review1)
      print("Inserted review with ID: \(review1.id)")
    }
    
    DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
      self.persistenceController.putReviews([review1, review2])
      print("Inserted reviews with IDs: \(review1.id), \(review2.id)")
    }
    
    // Delete review
    DispatchQueue.main.asyncAfter(deadline: .now() + 4) {
      self.persistenceController.deleteReview(id: review1.id)
      print("Deleted review with ID: \(review1.id)")
    }
    
    DispatchQueue.main.asyncAfter(deadline: .now() + 5) {
      self.persistenceController.putReviews([review1, review2])
      print("Inserted reviews with IDs: \(review1.id), \(review2.id)")
    }
    
    // Delete multiple reviews
    DispatchQueue.main.asyncAfter(deadline: .now() + 6) {
      self.persistenceController.deleteReviews(ids: [review1.id, review2.id])
      print("Deleted reviews with IDs: \(review1.id), \(review2.id)")
    }
    
    DispatchQueue.main.asyncAfter(deadline: .now() + 7) {
      self.persistenceController.putReviews([review1, review2, review3])
      print("Inserted reviews with IDs: \(review1.id), \(review2.id), \(review3.id)")
    }
    
    // Delete all reviews for a place
    DispatchQueue.main.asyncAfter(deadline: .now() + 8) {
      self.persistenceController.deleteAllPlaceReviews(placeId: 1)
      print("Deleted all reviews for place with ID: 1")
    }
    
    DispatchQueue.main.asyncAfter(deadline: .now() + 9) {
      self.persistenceController.putReviews([review1, review2, review3])
      print("Inserted reviews with IDs: \(review1.id), \(review2.id), \(review3.id)")
    }
    
    // Delete all reviews
    DispatchQueue.main.asyncAfter(deadline: .now() + 10) {
      self.persistenceController.deleteAllReviews()
      print("Deleted all reviews")
    }
  }
  
  private static func testObserveReviewsForPlace() {
    print("Testing Observe Reviews for Place...")
    persistenceController.reviewsForPlaceSubject
      .sink(receiveCompletion: { completion in
        if case .failure(let error) = completion {
          print("Observe reviews failed with error: \(error)")
        }
      }, receiveValue: { reviews in
        print("Reviews for Place Results:")
        reviews.forEach { review in
          print("ID: \(review.id), PlaceID: \(review.placeId), Rating: \(review.rating), Comment: \(review.comment ?? "No comment"), deletionPlanned: \(review.deletionPlanned)")
        }
      })
      .store(in: &cancellables)
    
    persistenceController.observeReviewsForPlace(placeId: 1)
  }
  
  private static func testMarkReviewForDeletion() {
    print("Testing Mark Review for Deletion...")
    DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
      self.persistenceController.markReviewForDeletion(id: 1, deletionPlanned: true)
      print("Marked review with ID 1 for deletion")
    }
    
    DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
      self.persistenceController.markReviewForDeletion(id: 1, deletionPlanned: false)
      print("Unmarked review with ID 1 for deletion")
    }
  }
  
  private static func testGetReviewsPlannedForDeletion() {
    print("Testing Get Reviews Planned for Deletion...")
    DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
      let reviewsPlannedForDeletion = self.persistenceController.getReviewsPlannedForDeletion()
      print("Reviews planned for deletion:")
      reviewsPlannedForDeletion.forEach { review in
        print("ID: \(review.id), PlaceID: \(review.placeId), Rating: \(review.rating), Comment: \(review.comment ?? "No comment"), deletionPlanned: \(review.deletionPlanned)")
      }
    }
  }
}
