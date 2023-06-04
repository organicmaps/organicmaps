# logging

PREPROCESS_LOG_FILE = 'preprocessing.log'
LOG_FILE = 'fail.log'


def clear_log_files
  Dir.chdir TEST_FOLDER do
    # emptying existing files, rather than deleting and writing new ones makes it 
    # easier to use tail -f from the command line
    `echo '' > #{OSRM_ROUTED_LOG_FILE}`
    `echo '' > #{PREPROCESS_LOG_FILE}`
    `echo '' > #{LOG_FILE}`
  end
end

def log s='', type=nil
  if type == :preprocess
    file = PREPROCESS_LOG_FILE
  else
    file = LOG_FILE
  end
  File.open(file, 'a') {|f| f.write("#{s}\n") }
end


def log_scenario_fail_info
  return if @has_logged_scenario_info
  log "========================================="
  log "Failed scenario: #{@scenario_title}"
  log "Time: #{@scenario_time}"
  log "Fingerprint osm stage: #{@fingerprint_osm}"
  log "Fingerprint extract stage: #{@fingerprint_extract}"
  log "Fingerprint prepare stage: #{@fingerprint_prepare}"
  log "Fingerprint route stage: #{@fingerprint_route}"
  log "Profile: #{@profile}"
  log
  log '```xml' #so output can be posted directly to github comment fields
  log osm_str.strip
  log '```'
  log
  log
  @has_logged_scenario_info = true
end

def log_fail expected,got,attempts
  return
  log_scenario_fail_info
  log "== "
  log "Expected: #{expected}"
  log "Got:      #{got}"
  log
  ['route','forw','backw'].each do |direction|
    if attempts[direction]
      attempts[direction]
      log "Direction: #{direction}"
      log "Query: #{attempts[direction][:query]}"
      log "Response: #{attempts[direction][:response].body}"
      log
    end
  end
end


def log_preprocess_info
  return if @has_logged_preprocess_info
  log "=========================================", :preprocess
  log "Preprocessing data for scenario: #{@scenario_title}", :preprocess
  log "Time: #{@scenario_time}", :preprocess
  log '', :preprocess
  log "== OSM data:", :preprocess
  log '```xml', :preprocess #so output can be posted directly to github comment fields
  log osm_str, :preprocess
  log '```', :preprocess
  log '', :preprocess
  log "== Profile:", :preprocess
  log @profile, :preprocess
  log '', :preprocess
  @has_logged_preprocess_info = true
end

def log_preprocess str
  log_preprocess_info
  log str, :preprocess
end

def log_preprocess_done
end
