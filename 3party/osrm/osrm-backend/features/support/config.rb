def profile
  @profile ||= reset_profile
end

def reset_profile
  @profile = nil
  set_profile DEFAULT_SPEEDPROFILE
end

def set_profile profile
  @profile = profile
end
