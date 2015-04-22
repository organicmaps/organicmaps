require 'OSM/objects'       #osmlib gem
require 'OSM/Database'
require 'builder'
require 'fileutils'

class Location
  attr_accessor :lon,:lat

  def initialize lon,lat
    @lat = lat
    @lon = lon
  end
end


def set_input_format format
  raise '*** Input format must be eiter "osm" or "pbf"' unless ['pbf','osm'].include? format.to_s
  @input_format = format.to_s
end

def input_format
  @input_format || DEFAULT_INPUT_FORMAT
end

def sanitized_scenario_title
  @sanitized_scenario_title ||= @scenario_title.to_s.gsub /[^0-9A-Za-z.\-]/, '_'
end

def set_grid_size meters
  #the constant is calculated (with BigDecimal as: 1.0/(DEG_TO_RAD*EARTH_RADIUS_IN_METERS
  #see ApproximateDistance() in ExtractorStructs.h
  #it's only accurate when measuring along the equator, or going exactly north-south
  @zoom = meters.to_f*0.8990679362704610899694577444566908445396483347536032203503E-5
end

def set_origin origin
  @origin = origin
end

def build_ways_from_table table
  #add one unconnected way for each row
  table.hashes.each_with_index do |row,ri|
    #NOTE:
    #currently osrm crashes when processing an isolated oneway with just 2 nodes, so we use 4 edges
    #this is relatated to the fact that a oneway dead-end street doesn't make a lot of sense

    #if we stack ways on different x coordinates, routability tests get messed up, because osrm might pick a neighboring way if the one test can't be used.
    #instead we place all lines as a string on the same y coordinate. this prevents using neightboring ways.

    #a few nodes...
    node1 = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, @origin[0]+(0+WAY_SPACING*ri)*@zoom, @origin[1]
    node2 = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, @origin[0]+(1+WAY_SPACING*ri)*@zoom, @origin[1]
    node3 = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, @origin[0]+(2+WAY_SPACING*ri)*@zoom, @origin[1]
    node4 = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, @origin[0]+(3+WAY_SPACING*ri)*@zoom, @origin[1]
    node5 = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, @origin[0]+(4+WAY_SPACING*ri)*@zoom, @origin[1]
    node1.uid = OSM_UID
    node2.uid = OSM_UID
    node3.uid = OSM_UID
    node4.uid = OSM_UID
    node5.uid = OSM_UID
    node1 << { :name => "a#{ri}" }
    node2 << { :name => "b#{ri}" }
    node3 << { :name => "c#{ri}" }
    node4 << { :name => "d#{ri}" }
    node5 << { :name => "e#{ri}" }

    osm_db << node1
    osm_db << node2
    osm_db << node3
    osm_db << node4
    osm_db << node5

    #...with a way between them
    way = OSM::Way.new make_osm_id, OSM_USER, OSM_TIMESTAMP
    way.uid = OSM_UID
    way << node1
    way << node2
    way << node3
    way << node4
    way << node5

    tags = row.dup

    # remove tags that describe expected test result
    tags.reject! do |k,v|
      k =~ /^forw\b/ ||
      k =~ /^backw\b/ ||
      k =~ /^bothw\b/
    end

    ##remove empty tags
    tags.reject! { |k,v| v=='' }

    # sort tag keys in the form of 'node/....'
    way_tags = { 'highway' => 'primary' }

    node_tags = {}
    tags.each_pair do |k,v|
      if k =~ /node\/(.*)/
        if v=='(nil)'
          node_tags.delete k
        else
          node_tags[$1] = v
        end
      else
        if v=='(nil)'
          way_tags.delete k
        else
          way_tags[k] = v
        end
      end
    end

    way_tags['name'] = "w#{ri}"
    way << way_tags
    node3 << node_tags

    osm_db << way
  end
end

def table_coord_to_lonlat ci,ri
  [@origin[0]+ci*@zoom, @origin[1]-ri*@zoom]
end

def add_osm_node name,lon,lat
  node = OSM::Node.new make_osm_id, OSM_USER, OSM_TIMESTAMP, lon, lat
  node << { :name => name }
  node.uid = OSM_UID
  osm_db << node
  name_node_hash[name] = node
end

def add_location name,lon,lat
  location_hash[name] = Location.new(lon,lat)
end

def find_node_by_name s
  raise "***invalid node name '#{s}', must be single characters" unless s.size == 1
  raise "*** invalid node name '#{s}', must be alphanumeric" unless s.match /[a-z0-9]/
  if s.match /[a-z]/
    from_node = name_node_hash[ s.to_s ]
  else
    from_node = location_hash[ s.to_s ]
  end
end

def find_way_by_name s
  name_way_hash[s.to_s] || name_way_hash[s.to_s.reverse]
end

def reset_data
  Dir.chdir TEST_FOLDER do
    #clear_log
    #clear_data_files
  end
  reset_profile
  reset_osm
  @fingerprint_osm = nil
  @fingerprint_extract = nil
  @fingerprint_prepare = nil
  @fingerprint_route = nil
end

def make_osm_id
  @osm_id = @osm_id+1
end

def reset_osm
  osm_db.clear
  name_node_hash.clear
  location_hash.clear
  name_way_hash.clear
  @osm_str = nil
  @osm_hash = nil
  @osm_id = 0
end

def clear_data_files
  File.delete *Dir.glob("#{DATA_FOLDER}/test.*")
end

def clear_log
  File.delete *Dir.glob("*.log")
end

def osm_db
  @osm_db ||= OSM::Database.new
end

def name_node_hash
  @name_node_hash ||= {}
end

def location_hash
  @location_hash ||= {}
end

def name_way_hash
  @name_way_hash ||= {}
end

def osm_str
  return @osm_str if @osm_str
  @osm_str = ''
  doc = Builder::XmlMarkup.new :indent => 2, :target => @osm_str
  doc.instruct!
  osm_db.to_xml doc, OSM_GENERATOR
  @osm_str
end

def osm_file
  @osm_file ||= "#{DATA_FOLDER}/#{fingerprint_osm}"
end

def extracted_file
  @extracted_file ||= "#{osm_file}_#{fingerprint_extract}"
end

def prepared_file
  @prepared_file ||= "#{osm_file}_#{fingerprint_extract}_#{fingerprint_prepare}"
end

def write_osm
  Dir.mkdir DATA_FOLDER unless File.exist? DATA_FOLDER
  unless File.exist?("#{osm_file}.osm")
    File.open( "#{osm_file}.osm", 'w') {|f| f.write(osm_str) }
  end
end

def convert_osm_to_pbf
  unless File.exist?("#{osm_file}.osm.pbf")
    log_preprocess_info
    log "== Converting #{osm_file}.osm to protobuffer format...", :preprocess
    unless system "osmosis --read-xml #{osm_file}.osm --write-pbf #{osm_file}.osm.pbf omitmetadata=true >>#{PREPROCESS_LOG_FILE} 2>&1"
      raise OsmosisError.new $?, "osmosis exited with code #{$?.exitstatus}"
    end
    log '', :preprocess
  end
end

def extracted?
  Dir.chdir TEST_FOLDER do
    File.exist?("#{extracted_file}.osrm") &&
    File.exist?("#{extracted_file}.osrm.names") &&
    File.exist?("#{extracted_file}.osrm.restrictions")
  end
end

def prepared?
  Dir.chdir TEST_FOLDER do
    File.exist?("#{prepared_file}.osrm.hsgr")
  end
end

def write_timestamp
  File.open( "#{prepared_file}.osrm.timestamp", 'w') {|f| f.write(OSM_TIMESTAMP) }
end

def pbf?
  input_format=='pbf'
end

def write_input_data
  Dir.chdir TEST_FOLDER do
    write_osm
    write_timestamp
    convert_osm_to_pbf if pbf?
  end
end

def extract_data
  Dir.chdir TEST_FOLDER do
    log_preprocess_info
    log "== Extracting #{osm_file}.osm...", :preprocess
    unless system "#{BIN_PATH}/osrm-extract #{osm_file}.osm#{'.pbf' if pbf?} --profile #{PROFILES_PATH}/#{@profile}.lua >>#{PREPROCESS_LOG_FILE} 2>&1"
      log "*** Exited with code #{$?.exitstatus}.", :preprocess
      raise ExtractError.new $?.exitstatus, "osrm-extract exited with code #{$?.exitstatus}."
    end
    begin
      ["osrm","osrm.names","osrm.restrictions"].each do |file|
        File.rename "#{osm_file}.#{file}", "#{extracted_file}.#{file}"
      end
    rescue Exception => e
      raise FileError.new nil, "failed to rename data file after extracting."
    end
  end
end

def prepare_data
  Dir.chdir TEST_FOLDER do
    log_preprocess_info
    log "== Preparing #{extracted_file}.osm...", :preprocess
    unless system "#{BIN_PATH}/osrm-prepare #{extracted_file}.osrm  --profile #{PROFILES_PATH}/#{@profile}.lua >>#{PREPROCESS_LOG_FILE} 2>&1"
      log "*** Exited with code #{$?.exitstatus}.", :preprocess
      raise PrepareError.new $?.exitstatus, "osrm-prepare exited with code #{$?.exitstatus}."
    end
    begin
      ["osrm.hsgr","osrm.fileIndex","osrm.geometry","osrm.nodes","osrm.ramIndex"].each do |file|
        File.rename "#{extracted_file}.#{file}", "#{prepared_file}.#{file}"
      end
    rescue Exception => e
      raise FileError.new nil, "failed to rename data file after preparing."
    end
    begin
      ["osrm.names","osrm.edges","osrm.restrictions"].each do |file|
        FileUtils.cp "#{extracted_file}.#{file}", "#{prepared_file}.#{file}"
      end
    rescue Exception => e
      raise FileError.new nil, "failed to copy data file after preparing."
    end
    log '', :preprocess
  end
end

def reprocess
  write_input_data
  extract_data unless extracted?
  prepare_data unless prepared?
  log_preprocess_done
end
