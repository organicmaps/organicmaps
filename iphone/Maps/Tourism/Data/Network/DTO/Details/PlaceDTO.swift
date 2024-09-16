import Foundation

struct PlaceDTO: Codable {
    let id: Int64
    let name: String
    let coordinates: CoordinatesDTO?
    let cover: String
    let feedbacks: [ReviewDTO]?
    let gallery: [String]
    let rating: Double
    let shortDescription: String
    let longDescription: String
    
    enum CodingKeys: String, CodingKey {
        case id, name, coordinates, cover, feedbacks, gallery, rating, shortDescription, longDescription
    }
    
    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        id = try container.decode(Int64.self, forKey: .id)
        name = try container.decode(String.self, forKey: .name)
        coordinates = try container.decodeIfPresent(CoordinatesDTO.self, forKey: .coordinates)
        cover = try container.decode(String.self, forKey: .cover)
        feedbacks = try container.decodeIfPresent([ReviewDTO].self, forKey: .feedbacks)
        gallery = try container.decode([String].self, forKey: .gallery)
        rating = try container.decode(FlexibleDouble.self, forKey: .rating).value
        shortDescription = try container.decode(String.self, forKey: .shortDescription)
        longDescription = try container.decode(String.self, forKey: .longDescription)
    }
    
    func toPlaceFull(isFavorite: Bool) -> PlaceFull {
        return PlaceFull(
            id: id,
            name: name,
            rating: rating,
            excerpt: shortDescription,
            description: longDescription,
            placeLocation: coordinates?.toPlaceLocation(name: name),
            cover: cover,
            pics: gallery,
            reviews: feedbacks?.map { $0.toReview() } ?? [],
            isFavorite: isFavorite
        )
    }
}

struct FlexibleDouble: Codable {
    let value: Double
    
    init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        if let doubleValue = try? container.decode(Double.self) {
            value = doubleValue
        } else if let stringValue = try? container.decode(String.self),
                  let doubleValue = Double(stringValue) {
            value = doubleValue
        } else {
            throw DecodingError.dataCorruptedError(in: container, debugDescription: "Unable to decode double value")
        }
    }
}
