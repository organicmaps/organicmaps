require 'safe_yaml/load'

SafeYAML::OPTIONS[:suppress_warnings] = true

module Twine
  class Plugin
    attr_reader :debug, :config

    def initialize
      @debug = false
      require_gems
    end

    ###
    # require gems from the yaml config.
    #
    # gems: [twine-plugin1, twine-2]
    #
    # also works with single gem
    #
    # gems: twine-plugin1
    #
    def require_gems
      # ./twine.yml    # current working directory
      # ~/.twine       # home directory
      # /etc/twine.yml # etc
      cwd_config  = join_path Dir.pwd, 'twine.yml'
      home_config = join_path Dir.home, '.twine'
      etc_config  = '/etc/twine.yml'

      config_order = [cwd_config, home_config, etc_config]

      puts "Config order: #{config_order}" if debug

      config_order.each do |config_file|
        next unless valid_file config_file
        puts "Loading: #{config_file}" if debug
        @config = SafeYAML.load_file config_file
        puts "Config yaml: #{config}" if debug
        break
      end

      return unless config

      # wrap gems in an array. if nil then array will be empty
      Kernel.Array(config['gems']).each do |gem_path|
        puts "Requiring: #{gem_path}" if debug
        require gem_path
      end
    end

    private

    def valid_file path
      File.exist?(path) && File.readable?(path) && !File.directory?(path)
    end

    def join_path *paths
      File.expand_path File.join *paths
    end
  end
end
