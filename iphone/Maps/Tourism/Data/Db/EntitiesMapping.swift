extension CurrencyRatesEntity {
  func toCurrencyRates() -> CurrencyRates {
    return CurrencyRates(usd: usd, eur: eur, rub: rub)
  }
}

extension PersonalDataEntity {
  func toPersonalData() -> PersonalData {
    return PersonalData(
      id: self.id,
      fullName: self.fullName ?? "",
      country: self.country ?? "",
      pfpUrl: self.pfpUrl,
      email: self.email ?? "",
      language: self.language,
      theme: self.theme
    )
  }
}

extension PlaceEntity {
  func toPlaceShort() -> PlaceShort {
    return PlaceShort(
      id: self.id,
      name: self.name ?? "",
      cover: self.cover,
      rating: self.rating,
      excerpt: self.excerpt,
      isFavorite: self.isFavorite
    )
  }
  
  func toPlaceFull() -> PlaceFull {
    let placeLocation =
    DBUtils.decodeFromJsonString(self.coordinatesJson ?? "", to: CoordinatesEntity.self)?.toPlaceLocation(name: self.name ?? "")
    let pics = DBUtils.decodeFromJsonString(self.galleryJson ?? "", to: [String].self) ?? []
    
    return PlaceFull(
      id: self.id,
      name: self.name ?? "",
      rating: self.rating,
      excerpt: self.excerpt ?? "",
      description: self.descr ?? "",
      placeLocation: placeLocation,
      cover: self.cover ?? "",
      pics: pics,
      // we have different table for reviews
      reviews: nil,
      isFavorite: self.isFavorite
    )
  }
}

extension [PlaceEntity] {
  func toFullPlaces() -> [PlaceFull] {
    return self.map { placeEntity in
      placeEntity.toPlaceFull()
    }
  }
  
  func toShortPlaces() -> [PlaceShort] {
    return self.map { placeEntity in
      placeEntity.toPlaceShort()
    }
  }
}

extension CoordinatesEntity {
  func toPlaceLocation(name: String) -> PlaceLocation {
    return PlaceLocation(name: name, lat: self.latitude, lon: self.longitude)
  }
}

extension UserEntity {
  func toUser() -> User {
    return User(
      id: self.userId,
      name: self.fullName,
      pfpUrl: self.avatar,
      countryCodeName: self.country
    )
  }
}
