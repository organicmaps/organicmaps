enum BMCSection {
  case permissions
  case categories
  case actions
  case notifications
}

protocol BMCModel {}

enum BMCPermission: BMCModel {
  case signup
  case backup
  case restore(Date?)
}

enum BMCAction: BMCModel {
  case create
}

enum BMCNotification: BMCModel {
  case load
}
