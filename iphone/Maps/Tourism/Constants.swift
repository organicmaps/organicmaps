struct Constants {
  // MARK: - Image Loading URLs
  static let imageUrlExample = "https://img.freepik.com/free-photo/young-woman-hiker-taking-photo-with-smartphone-on-mountains-peak-in-winter_335224-427.jpg?w=2000"
  static let thumbnailUrlExample = "https://render.fineartamerica.com/images/images-profile-flow/400/images-medium-large-5/awesome-solitude-bess-hamiti.jpg"
  static let logoUrlExample = "https://brandeps.com/logo-download/O/OSCE-logo-vector-01.svg"
  static let anotherImageExample = "https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcSiceDsFSiLmW2Jl-XP3m5UXRdyLRKBQTlPGQ&s"
  
  static let reviewExample = Review(
    id: 1,
    placeId: 1,
    rating: 5,
    user: User(id: 1, name: "John Doe", pfpUrl: Constants.imageUrlExample, countryCodeName: "US"),
    date: "2024-09-01",
    comment: "Amazing place! The views are incredible and the atmosphere is so calming.The views are incredible and the atmosphere is so calming.The views are incredible and the atmosphere is so calming.The views are incredible and the atmosphere is so calming.The views are incredible and the atmosphere is so calming.The views are incredible and the atmosphere is so calming.",
    picsUrls: [
      Constants.imageUrlExample,
      Constants.thumbnailUrlExample,
      Constants.imageUrlExample,
      Constants.thumbnailUrlExample,
      Constants.imageUrlExample,
      Constants.thumbnailUrlExample
    ]
  )
  
  // MARK: - Data
  static let categories: [String: String] = [
    "sights": NSLocalizedString("sights", comment: ""),
    "restaurants": NSLocalizedString("restaurants", comment: ""),
    "hotels_tourism": NSLocalizedString("hotels_tourism", comment: "")
  ]
  
  static let placeExample = PlaceFull(
    id: 1,
    name: "Beautiful Place",
    rating: 4.5,
    excerpt: "<p>Ресторан отличается от других подобных объектов bla bla bla.</p>",
    description: """
        <p>Ресторан отличается от других подобных объектов уникальным дизайном и новым способом подачи различных блюд. Красивое оформление холла здания, использование экологически чистых материалов и приятная музыка отражают теплый климат Бразилии.</p><p>Объект построен по франчайзинговому проекту, то есть на основе подряда, с использованием известного иностранного бренда и его технологий. Управлением рестораном и приготовлением различных блюд занимается гражданин Бразилии, который будет обслуживать клиентов в сотрудничестве с таджикскими поварами и официантами.</p>
    """,
    placeLocation: PlaceLocation(
      name: "Mountain Retreat",
      lat: 38.550288,
      lon: 68.729752
    ),
    cover: Constants.imageUrlExample,
    pics: [
      Constants.imageUrlExample,
      Constants.thumbnailUrlExample,
      Constants.imageUrlExample,
      Constants.imageUrlExample,
      Constants.anotherImageExample
    ],
    reviews: [
      Review(
        id: 1,
        placeId: 1,
        rating: 5,
        user: User(id: 1, name: "John Doe", pfpUrl: Constants.imageUrlExample, countryCodeName: "us"),
        date: "2024-09-01",
        comment: "Amazing place! The views are incredible and the atmosphere is so calming.",
        picsUrls: [
          Constants.imageUrlExample,
          Constants.thumbnailUrlExample
        ]
      ),
      Review(
        id: 2,
        placeId: 1,
        rating: 4,
        user: User(id: 2, name: "Jane Smith", pfpUrl: Constants.imageUrlExample, countryCodeName: "tj"),
        date: "2024-08-20",
        comment: "Great place for a weekend getaway. A bit crowded but worth the visit.",
        picsUrls: [
          Constants.imageUrlExample
        ]
      )
    ],
    isFavorite: false
  )
}

let BASE_URL_WITHOUT_API = "https://tourismmap.tj/"
let BASE_URL = "https://tourismmap.tj/api/"
