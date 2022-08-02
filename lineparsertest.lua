local lpeg = require 'lpeg'
lpeg.locale(lpeg) 

math.randomseed(os.time())

offset = 23
a = lpeg
a = offset+(math.random()*100)%(90-offset)