require 'digest/sha1'

bin_extract_hash = nil
profile_hashes = nil

def hash_of_files paths
  paths = [paths] unless paths.is_a? Array
  hash = Digest::SHA1.new
  for path in paths do
    open(path,'rb') do |io|
      while !io.eof
        buf = io.readpartial 1024
        hash.update buf
      end
    end
  end
  return hash.hexdigest
end


def profile_hash
  profile_hashes ||= {}
  profile_hashes[@profile] ||= hash_of_files "#{PROFILES_PATH}/#{@profile}.lua"
end

def osm_hash
  @osm_hash ||= Digest::SHA1.hexdigest osm_str
end

def lua_lib_hash
  @lua_lib_hash ||= hash_of_files Dir.glob("../profiles/lib/*.lua")
end

def bin_extract_hash
  bin_extract_hash ||= hash_of_files "#{BIN_PATH}/osrm-extract#{EXE}"
end

def bin_prepare_hash
  bin_prepare_hash ||= hash_of_files "#{BIN_PATH}/osrm-prepare#{EXE}"
end

def bin_routed_hash
  bin_routed_hash ||= hash_of_files "#{BIN_PATH}/osrm-routed#{EXE}"
end

#combine state of data, profile and binaries into a hash that identifies the exact test scenario
def fingerprint
  @fingerprint ||= Digest::SHA1.hexdigest "#{bin_extract_hash}-#{bin_prepare_hash}-#{bin_routed_hash}-#{profile_hash}-#{lua_lib_hash}-#{osm_hash}"
end
