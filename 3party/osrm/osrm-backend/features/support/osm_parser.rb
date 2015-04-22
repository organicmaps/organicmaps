require 'OSM/StreamParser'

locations = nil

class OSMTestParserCallbacks < OSM::Callbacks
  locations = nil

  def self.locations
    if locations
      locations
    else
      #parse the test file, so we can later reference nodes and ways by name in tests
      locations = {}
      file = 'test/data/test.osm'
      callbacks = OSMTestParserCallbacks.new
      parser = OSM::StreamParser.new(:filename => file, :callbacks => callbacks)
      parser.parse
      puts locations
    end
  end

  def node(node)
    locations[node.name] = [node.lat,node.lon]
  end
end