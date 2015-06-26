require 'twine/formatters/abstract'
require 'twine/formatters/android'
require 'twine/formatters/apple'
require 'twine/formatters/flash'
require 'twine/formatters/gettext'
require 'twine/formatters/jquery'
require 'twine/formatters/django'
require 'twine/formatters/tizen'

module Twine
  module Formatters
    @formatters = [Formatters::Apple, Formatters::Android, Formatters::Gettext, Formatters::JQuery, Formatters::Flash, Formatters::Django, Formatters::Tizen]

    class << self
      attr_reader :formatters

      ###
      # registers a new formatter
      #
      # formatter_class - the class of the formatter to register
      #
      # returns array of active formatters
      #
      def register_formatter formatter_class
        raise "#{formatter_class} already registered" if @formatters.include? formatter_class
        @formatters << formatter_class
      end
    end
  end
end