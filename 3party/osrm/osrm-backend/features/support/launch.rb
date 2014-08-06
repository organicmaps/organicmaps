require 'socket'
require 'open3'

if ENV['OS']==/Windows.*/ then
  TERMSIGNAL='TERM'
else
  TERMSIGNAL=9
end

OSRM_ROUTED_LOG_FILE = 'osrm-routed.log'

class OSRMBackgroundLauncher
  def initialize input_file, &block
    @input_file = input_file
    Dir.chdir TEST_FOLDER do
      begin
        launch
        yield
      ensure
        shutdown
      end
    end
  end

  private

  def launch
    Timeout.timeout(OSRM_TIMEOUT) do
      osrm_up
      wait_for_connection
    end
  rescue Timeout::Error
    raise RoutedError.new "Launching osrm-routed timed out."
  end

  def shutdown
    Timeout.timeout(OSRM_TIMEOUT) do
      osrm_down
    end
  rescue Timeout::Error
    kill
    raise RoutedError.new "Shutting down osrm-routed timed out."
  end


  def osrm_up?
    if @pid
      begin
        if Process.waitpid(@pid, Process::WNOHANG) then
           false
        else
           true
        end
      rescue Errno::ESRCH, Errno::ECHILD
        false
      end
    end
  end

  def osrm_up
    return if osrm_up?
    @pid = Process.spawn("#{BIN_PATH}/osrm-routed #{@input_file} --port #{OSRM_PORT}",:out=>OSRM_ROUTED_LOG_FILE, :err=>OSRM_ROUTED_LOG_FILE)
    Process.detach(@pid)    # avoid zombie processes
  end

  def osrm_down
    if @pid
      Process.kill TERMSIGNAL, @pid
      wait_for_shutdown
    end
  end

  def kill
    if @pid
      Process.kill 'KILL', @pid
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
      sleep 0.1
    end
  end
end
