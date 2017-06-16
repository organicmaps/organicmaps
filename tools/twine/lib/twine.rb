module Twine
  @@stdout = STDOUT
  @@stderr = STDERR

  def self.stdout
    @@stdout
  end

  def self.stdout=(out)
    @@stdout = out
  end

  def self.stderr
    @@stderr
  end

  def self.stderr=(err)
    @@stderr = err
  end

  class Error < StandardError
  end

  require 'twine/version'
  require 'twine/plugin'
  require 'twine/twine_file'
  require 'twine/encoding'
  require 'twine/output_processor'
  require 'twine/placeholders'
  require 'twine/formatters'
  require 'twine/formatters/abstract'
  require 'twine/formatters/android'
  require 'twine/formatters/apple'
  require 'twine/formatters/django'
  require 'twine/formatters/flash'
  require 'twine/formatters/gettext'
  require 'twine/formatters/jquery'
  require 'twine/formatters/tizen'
  require 'twine/runner'
  require 'twine/cli'
end
