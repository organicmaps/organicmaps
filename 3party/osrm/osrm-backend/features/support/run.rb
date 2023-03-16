def run_bin bin, options
  Dir.chdir TEST_FOLDER do
    opt = options.dup

    if opt.include? '{osm_base}'
      raise "*** {osm_base} is missing" unless osm_file
      opt.gsub! "{osm_base}", "#{osm_file}" 
    end

    if opt.include? '{extracted_base}'
      raise "*** {extracted_base} is missing" unless extracted_file
      opt.gsub! "{extracted_base}", "#{extracted_file}" 
    end

    if opt.include? '{prepared_base}'
      raise "*** {prepared_base} is missing" unless prepared_file
      opt.gsub! "{prepared_base}", "#{prepared_file}" 
    end
    if opt.include? '{profile}'
      opt.gsub! "{profile}", "#{PROFILES_PATH}/#{@profile}.lua" 
    end

    cmd = "#{QQ}#{BIN_PATH}/#{bin}#{EXE}#{QQ} #{opt} 2>error.log"
    @stdout = `#{cmd}`
    @stderr = File.read 'error.log'
    @exit_code = $?.exitstatus
  end
end