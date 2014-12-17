local math = math

module "MaxSpeed"

function limit(way,max,maxf,maxb)
  if maxf and maxf>0 then
    way.forward_speed = math.min(way.forward_speed, maxf)
  elseif max and max>0 then
    way.forward_speed = math.min(way.forward_speed, max)
  end

  if maxb and maxb>0 then
    way.backward_speed = math.min(way.backward_speed, maxb)
  elseif max and max>0 then
    way.backward_speed = math.min(way.backward_speed, max)
  end
end
