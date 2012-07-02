require 'twine/formatters/abstract'
require 'twine/formatters/android'
require 'twine/formatters/apple'
require 'twine/formatters/jquery'

module Twine
  module Formatters
    FORMATTERS = [Formatters::Apple, Formatters::Android, Formatters::JQuery]
  end
end
