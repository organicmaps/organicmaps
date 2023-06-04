#monkey-patch osmlib to fix a bug

module OSM
  class Way
    def to_xml(xml)
      xml.way(attributes) do
        nodes.each do |node|
          xml.nd(:ref => node)
        end
        tags.to_xml(xml)
      end
    end
  end
end
