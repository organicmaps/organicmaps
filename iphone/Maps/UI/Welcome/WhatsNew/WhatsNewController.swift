class WhatsNewController: WelcomeViewController {
  class var key: String { return WhatsNewBuilder.configs.reduce("\(self)", { return "\($0)_\($1.title)" }) }
}
