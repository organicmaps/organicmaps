Given /^the profile "([^"]*)"$/ do |profile|
  set_profile profile
end

Given(/^the import format "(.*?)"$/) do |format|
  set_input_format format
end

Given /^a grid size of (\d+) meters$/ do |meters|
  set_grid_size meters
end

Given /^the origin ([-+]?[0-9]*\.?[0-9]+),([-+]?[0-9]*\.?[0-9]+)$/ do |lat,lon|
  set_origin [lon.to_f,lat.to_f]
end

Given /^the shortcuts$/ do |table|
  table.hashes.each do |row|
    shortcuts_hash[ row['key'] ] = row['value']
  end
end

Given /^the node map$/ do |table|
  table.raw.each_with_index do |row,ri|
    row.each_with_index do |name,ci|
      unless name.empty?
        raise "*** node invalid name '#{name}', must be single characters" unless name.size == 1
        raise "*** invalid node name '#{name}', must me alphanumeric" unless name.match /[a-z0-9]/
        if name.match /[a-z]/
          raise "*** duplicate node '#{name}'" if name_node_hash[name]
          add_osm_node name, *table_coord_to_lonlat(ci,ri)
        else
          raise "*** duplicate node '#{name}'" if location_hash[name]
          add_location name, *table_coord_to_lonlat(ci,ri)
        end
      end
    end
  end
end

Given /^the node locations$/ do |table|
  table.hashes.each do |row|
    name = row['node']
    raise "*** duplicate node '#{name}'" if find_node_by_name name
    if name.match /[a-z]/
      add_osm_node name, row['lon'].to_f, row['lat'].to_f
    else
      add_location name, row['lon'].to_f, row['lat'].to_f
    end
  end
end

Given /^the nodes$/ do |table|
  table.hashes.each do |row|
    name = row.delete 'node'
    node = find_node_by_name(name)
    raise "*** unknown node '#{c}'" unless node
    node << row
  end
end

Given /^the ways$/ do |table|
  raise "*** Map data already defined - did you pass an input file in this scenaria?" if @osm_str
  table.hashes.each do |row|
    way = OSM::Way.new make_osm_id, OSM_USER, OSM_TIMESTAMP
    way.uid = OSM_UID

    nodes = row.delete 'nodes'
    raise "*** duplicate way '#{nodes}'" if name_way_hash[nodes]
    nodes.each_char do |c|
      raise "*** ways can only use names a-z, '#{name}'" unless c.match /[a-z]/
      node = find_node_by_name(c)
      raise "*** unknown node '#{c}'" unless node
      way << node
    end

    defaults = { 'highway' => 'primary' }
    tags = defaults.merge(row)

    if row['highway'] == '(nil)'
      tags.delete 'highway'
    end

    if row['name'] == nil
      tags['name'] = nodes
    elsif (row['name'] == '""') || (row['name'] == "''")
      tags['name'] = ''
    elsif row['name'] == '' || row['name'] == '(nil)'
      tags.delete 'name'
    else
      tags['name'] = row['name']
    end

    way << tags
    osm_db << way
    name_way_hash[nodes] = way
  end
end

Given /^the relations$/ do |table|
  raise "*** Map data already defined - did you pass an input file in this scenaria?" if @osm_str
  table.hashes.each do |row|
    relation = OSM::Relation.new make_osm_id, OSM_USER, OSM_TIMESTAMP
    row.each_pair do |key,value|
      if key =~ /^node:(.*)/
        value.split(',').map { |v| v.strip }.each do |node_name|
          raise "***invalid relation node member '#{node_name}', must be single character" unless node_name.size == 1
          node = find_node_by_name(node_name)
          raise "*** unknown relation node member '#{node_name}'" unless node
          relation << OSM::Member.new( 'node', node.id, $1 )
        end
      elsif key =~ /^way:(.*)/
        value.split(',').map { |v| v.strip }.each do |way_name|
          way = find_way_by_name(way_name)
          raise "*** unknown relation way member '#{way_name}'" unless way
          relation << OSM::Member.new( 'way', way.id, $1 )
        end
      elsif key =~ /^(.*):(.*)/
        raise "*** unknown relation member type '#{$1}', must be either 'node' or 'way'"
      else
        relation << { key => value }
      end
    end
    relation.uid = OSM_UID
    osm_db << relation
  end
end

Given /^the defaults$/ do
end

Given /^the input file ([^"]*)$/ do |file|
  raise "*** Input file must in .osm format" unless File.extname(file)=='.osm'
  @osm_str = File.read file
end

Given /^the data has been saved to disk$/ do
  begin
    write_input_data
  rescue OSRMError => e
    @process_error = e
  end
end

Given /^the data has been extracted$/ do
  begin
    write_input_data
    extract_data unless extracted?
  rescue OSRMError => e
    @process_error = e
  end
end

Given /^the data has been prepared$/ do
  begin
    reprocess
  rescue OSRMError => e
    @process_error = e
  end
end

Given /^osrm\-routed is stopped$/ do
  begin
    OSRMLoader.shutdown
  rescue OSRMError => e
    @process_error = e
  end
end

Given /^data is loaded directly/ do
  @load_method = 'directly'
end

Given /^data is loaded with datastore$/ do
  @load_method = 'datastore'
end
