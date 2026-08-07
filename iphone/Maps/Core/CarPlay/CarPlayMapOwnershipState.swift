enum CarPlayMapDisplay: Equatable {
  case device
  case mainCarPlay
  case dashboardCarPlay
}

enum CarPlayMapSelection: Equatable {
  case phone
  case car
}

struct CarPlayMapAvailability: Equatable {
  var isDeviceConnected = false
  var isMainCarPlayConnected = false
  var isDashboardConnected = false
  var isMainCarPlayVisible = false
  var activeCarPlayDisplay: CarPlayMapDisplay?

  func contains(_ display: CarPlayMapDisplay) -> Bool {
    switch display {
    case .device:
      return isDeviceConnected
    case .mainCarPlay:
      return isMainCarPlayConnected
    case .dashboardCarPlay:
      return isDashboardConnected
    }
  }
}

struct CarPlayMapOwnershipState: Equatable {
  private(set) var attachedDisplay: CarPlayMapDisplay = .device
  private(set) var selection: CarPlayMapSelection = .car

  var isPhoneSelected: Bool {
    selection == .phone
  }

  mutating func selectPhone() {
    selection = .phone
  }

  mutating func selectCar() {
    selection = .car
  }

  mutating func phoneDidDisconnect() {
    guard selection == .phone else { return }
    selection = .car
  }

  mutating func didAttach(to display: CarPlayMapDisplay) {
    attachedDisplay = display
  }

  mutating func reset() {
    attachedDisplay = .device
    selection = .car
  }

  func desiredDisplay(for availability: CarPlayMapAvailability) -> CarPlayMapDisplay? {
    if selection == .phone, availability.isDeviceConnected {
      return .device
    }

    if let carPlayDisplay = preferredCarPlayDisplay(for: availability) {
      return carPlayDisplay
    }
    return availability.isDeviceConnected ? .device : nil
  }

  private func preferredCarPlayDisplay(for availability: CarPlayMapAvailability) -> CarPlayMapDisplay? {
    if let activeCarPlayDisplay = availability.activeCarPlayDisplay,
       activeCarPlayDisplay != .device,
       availability.contains(activeCarPlayDisplay) {
      return activeCarPlayDisplay
    }

    if availability.isMainCarPlayConnected,
       availability.isMainCarPlayVisible || !availability.isDashboardConnected {
      return .mainCarPlay
    }
    if availability.isDashboardConnected {
      return .dashboardCarPlay
    }
    return nil
  }
}
