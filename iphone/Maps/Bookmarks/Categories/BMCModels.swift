enum BMCSection {
  case categories
  case actions
  case notifications
}

protocol BMCModel {}

enum BMCAction: BMCModel {
  case create
}

enum BMCNotification: BMCModel {
  case load
}
