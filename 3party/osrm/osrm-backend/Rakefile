require 'OSM/StreamParser'
require 'socket'
require 'digest/sha1'
require 'cucumber/rake/task'
require 'sys/proctable'

BUILD_FOLDER = 'build'
DATA_FOLDER = 'sandbox'
PROFILE = 'bicycle'
OSRM_PORT = 5000
PROFILES_FOLDER = '../profiles'

Cucumber::Rake::Task.new do |t|
  t.cucumber_opts = %w{--format pretty}
end

areas = {
  :kbh => { :country => 'denmark', :bbox => 'top=55.6972 left=12.5222 right=12.624 bottom=55.6376' },
  :frd => { :country => 'denmark', :bbox => 'top=55.7007 left=12.4765 bottom=55.6576 right=12.5698' },
  :regh => { :country => 'denmark', :bbox => 'top=56.164 left=11.792 bottom=55.403 right=12.731' },
  :denmark => { :country => 'denmark', :bbox => nil },
  :skaane => { :country => 'sweden', :bbox => 'top=56.55 left=12.4 bottom=55.3 right=14.6' }
}



osm_data_area_name = ARGV[1] ? ARGV[1].to_s.to_sym : :kbh
raise "Unknown data area." unless areas[osm_data_area_name]
osm_data_country = areas[osm_data_area_name][:country]
osm_data_area_bbox = areas[osm_data_area_name][:bbox]


task osm_data_area_name.to_sym {}   #define empty task to prevent rake from whining. will break if area has same name as a task


def each_process name, &block
  Sys::ProcTable.ps do |process|
    if process.comm.strip == name.strip && process.state != 'zombie'
      yield process.pid.to_i, process.state.strip
    end
  end
end

def up?
  find_pid('osrm-routed') != nil
end

def find_pid name
  each_process(name) { |pid,state| return pid.to_i }
  return nil
end

def wait_for_shutdown name
  timeout = 10
  (timeout*10).times do
    return if find_pid(name) == nil
    sleep 0.1
  end
  raise "*** Could not terminate #{name}."
end


desc "Rebuild and run tests."
task :default => [:build]

desc "Build using CMake."
task :build do
  if Dir.exists? BUILD_FOLDER
    Dir.chdir BUILD_FOLDER do
      system "make"
    end
  else
    system "mkdir build; cd build; cmake ..; make"
  end
end

desc "Setup config files."
task :setup do
end

desc "Download OSM data."
task :download do
  Dir.mkdir "#{DATA_FOLDER}" unless File.exist? "#{DATA_FOLDER}"
  puts "Downloading..."
  puts "curl http://download.geofabrik.de/europe/#{osm_data_country}-latest.osm.pbf -o #{DATA_FOLDER}/#{osm_data_country}.osm.pbf"
  raise "Error while downloading data." unless system "curl http://download.geofabrik.de/europe/#{osm_data_country}-latest.osm.pbf -o #{DATA_FOLDER}/#{osm_data_country}.osm.pbf"
  if osm_data_area_bbox
    puts "Cropping and converting to protobuffer..."
    raise "Error while cropping data." unless system "osmosis --read-pbf file=#{DATA_FOLDER}/#{osm_data_country}.osm.pbf --bounding-box #{osm_data_area_bbox} --write-pbf file=#{DATA_FOLDER}/#{osm_data_area_name}.osm.pbf omitmetadata=true"
  end
end

desc "Crop OSM data"
task :crop do
  if osm_data_area_bbox
    raise "Error while cropping data." unless system "osmosis --read-pbf file=#{DATA_FOLDER}/#{osm_data_country}.osm.pbf --bounding-box #{osm_data_area_bbox} --write-pbf file=#{DATA_FOLDER}/#{osm_data_area_name}.osm.pbf omitmetadata=true"
  end
end

desc "Reprocess OSM data."
task :process => [:extract,:prepare] do
end

desc "Extract OSM data."
task :extract do
  Dir.chdir DATA_FOLDER do
    raise "Error while extracting data." unless system "../#{BUILD_FOLDER}/osrm-extract #{osm_data_area_name}.osm.pbf --profile ../profiles/#{PROFILE}.lua"
  end
end

desc "Prepare OSM data."
task :prepare do
  Dir.chdir DATA_FOLDER do
    raise "Error while preparing data." unless system "../#{BUILD_FOLDER}/osrm-prepare #{osm_data_area_name}.osrm --profile ../profiles/#{PROFILE}.lua"
  end
end

desc "Delete preprocessing files."
task :clean do
  File.delete *Dir.glob("#{DATA_FOLDER}/*.osrm")
  File.delete *Dir.glob("#{DATA_FOLDER}/*.osrm.*")
end

desc "Run all cucumber test"
task :test do
  system "cucumber"
  puts
end

desc "Run the routing server in the terminal. Press Ctrl-C to stop."
task :run do
  Dir.chdir DATA_FOLDER do
    system "../#{BUILD_FOLDER}/osrm-routed #{osm_data_area_name}.osrm --port #{OSRM_PORT}"
  end
end

desc "Launch the routing server in the background. Use rake:down to stop it."
task :up do
  Dir.chdir DATA_FOLDER do
    abort("Already up.") if up?
    pipe = IO.popen("../#{BUILD_FOLDER}/osrm-routed #{osm_data_area_name}.osrm --port #{OSRM_PORT} 1>>osrm-routed.log 2>>osrm-routed.log")
    timeout = 5
    (timeout*10).times do
      begin
        socket = TCPSocket.new('localhost', OSRM_PORT)
        socket.puts 'ping'
      rescue Errno::ECONNREFUSED
        sleep 0.1
      end
    end
  end
end

desc "Stop the routing server."
task :down do
  pid = find_pid 'osrm-routed'
  if pid
    Process.kill 'TERM', pid
  else
    puts "Already down."
  end 
end

desc "Kill all osrm-extract, osrm-prepare and osrm-routed processes."
task :kill do
  each_process('osrm-routed') { |pid,state| Process.kill 'KILL', pid }
  each_process('osrm-prepare') { |pid,state| Process.kill 'KILL', pid }
  each_process('osrm-extract') { |pid,state| Process.kill 'KILL', pid }
  wait_for_shutdown 'osrm-routed'
  wait_for_shutdown 'osrm-prepare'
  wait_for_shutdown 'osrm-extract'  
end

desc "Get PIDs of all osrm-extract, osrm-prepare and osrm-routed processes."
task :pid do
  each_process 'osrm-routed' do |pid,state|
    puts "#{pid}\t#{state}"
  end
end

desc "Stop, reprocess and restart."
task :update => [:down,:process,:up] do
end


desc "Remove test cache files."
task :sweep do
  system "rm test/cache/*"
end

