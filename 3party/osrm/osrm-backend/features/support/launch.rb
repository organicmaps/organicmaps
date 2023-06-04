require 'socket'
require 'open3'
require 'json'

# Only one isntance of osrm-routed is ever launched, to avoid collisions.
# The default is to keep osrm-routed running and load data with datastore.
# however, osrm-routed it shut down and relaunched for each scenario thats
# loads data directly.
class OSRMLoader

  class OSRMBaseLoader
    @@pid = nil

    def launch
      Timeout.timeout(LAUNCH_TIMEOUT) do
        osrm_up
        wait_for_connection
      end
    rescue Timeout::Error
      raise RoutedError.new "Launching osrm-routed timed out."
    end

    def shutdown
      Timeout.timeout(SHUTDOWN_TIMEOUT) do
        osrm_down
      end
    rescue Timeout::Error
      kill
      raise RoutedError.new "Shutting down osrm-routed timed out."
    end

    def osrm_up?
      if @@pid
        begin
          if Process.waitpid(@@pid, Process::WNOHANG) then
             false
          else
             true
          end
        rescue Errno::ESRCH, Errno::ECHILD
          false
        end
      end
    end

    def osrm_down
      if @@pid
        Process.kill TERMSIGNAL, @@pid
        wait_for_shutdown
        @@pid = nil
      end
    end

    def kill
      if @@pid
        Process.kill 'KILL', @@pid
      end
    end

    def wait_for_connection
      while true
        begin
          socket = TCPSocket.new('127.0.0.1', OSRM_PORT)
          return
        rescue Errno::ECONNREFUSED
          sleep 0.1
        end
      end
    end

    def wait_for_shutdown
      while osrm_up?
        sleep 0.01
      end
    end
  end

  # looading data directly when lauching osrm-routed:
  # under this scheme, osmr-routed is launched and shutdown for each scenario,
  # and osrm-datastore is not used
  class OSRMDirectLoader < OSRMBaseLoader
    def load world, input_file, &block
      @world = world
      @input_file = input_file
      Dir.chdir TEST_FOLDER do
        shutdown
        launch
        yield
        shutdown
      end
    end

    def osrm_up
      return if @@pid
      @@pid = Process.spawn("#{BIN_PATH}/osrm-routed #{@input_file} --port #{OSRM_PORT}",:out=>OSRM_ROUTED_LOG_FILE, :err=>OSRM_ROUTED_LOG_FILE)
      Process.detach(@@pid)    # avoid zombie processes
    end

  end

  # looading data with osrm-datastore:
  # under this scheme, osmr-routed is launched once and kept running for all scenarios,
  # and osrm-datastore is used to load data for each scenario
  class OSRMDatastoreLoader < OSRMBaseLoader
    def load world, input_file, &block
      @world = world
      @input_file = input_file
      Dir.chdir TEST_FOLDER do
        load_data
        launch unless @@pid
        yield
      end
    end

    def load_data
      run_bin "osrm-datastore", @input_file
    end

    def osrm_up
      return if osrm_up?
      @@pid = Process.spawn("#{BIN_PATH}/osrm-routed --shared-memory=1 --port #{OSRM_PORT}",:out=>OSRM_ROUTED_LOG_FILE, :err=>OSRM_ROUTED_LOG_FILE)
      Process.detach(@@pid)    # avoid zombie processes
    end
  end

  def self.load world, input_file, &block
    method = world.instance_variable_get "@load_method"
    if method == 'datastore'
      OSRMDatastoreLoader.new.load world, input_file, &block
    elsif method == 'directly'
      OSRMDirectLoader.new.load world, input_file, &block
    else
      raise "*** Unknown load method '#{method}'"
    end
  end

end
