local lpeg = require 'lpeg'
lpeg.locale(lpeg) 

function splitNoteAmp(s,sep)
  sep = lpeg.P(sep)
  local elem = lpeg.C((1 - sep)^0)
  local p = elem * (sep * elem)^0
  return lpeg.match(p, s)
end

-- function listSplitNoteAmp(...)
--   local arg = {...}
--   for i,v in ipairs(arg) do
--     print(splitNoteAmp(v,"~"))
--   end
-- end

function addToChordTable(...)
  result = {}
  local arg = {...}
  for i,v in ipairs(arg) do
    result[#result+1] = v
  end
  return {result}
end

-- function addToNoteTable(...)
--   result = {}
--   local arg = {...}
--   for i,v in ipairs(arg) do
--     result[#result+1] = {v}
--   end
--   return result
-- end

-- function toAmp(v)
--   print("amp",v)
--   -- if string.find(v,"~") then
--     a,b = splitNoteAmp(v,"~")
--     -- print("a",a,"b",b)
--     -- if b == nil then b = 1 end
    
--     b = tonumber(b)
--     print("b",b)
--   -- else
--   --   -- print("a",a,"b",b)
--   --   a = v
--   --   b = tonumber(1)
--   -- end
--   -- print("a",a,"b",b)
--   return b
-- end

-- function addToNoteTable2(...)
--   result = {}
--   local arg = {...}
--   for i,v in ipairs(arg) do
--     if string.find(v,"~") then
--       a,b = splitNoteAmp(v,"~")
--       if b == nil then b = 1 end
--       result[#result+1] = {{tonumber(a),tonumber(b)}}
--       -- print("a",result[#result][1][1],"b",result[#result][1][2])
--     --[[else 
--       -- print("a",a,"b",b)
--       print("v->",v)
--       if string.find(v," ") then
--         print("In")
--         a,b = splitNoteAmp(v," ")
--         -- print("a",a,"b",b)
--         -- c = b
--         -- b = 1
--         _ra = {}
--         _ra[1] = {{tonumber(a),tonumber(1)}}; _ra[2] = {{tonumber(b),tonumber(1)}}
--         result[#result+1] = _ra[1]
--         print("a",result[#result][1][1],"b",result[#result][1][2])
--         result[#result+1] = _ra[2]
--         print("a",result[#result][1][1],"b",result[#result][1][2])
--     end]]
--     else
--       -- print("a",a,"b",b)
--       a = v
--       b = 1
--       result[#result+1] = {{tonumber(a),tonumber(b)}}
--       print("a",a,"b",b)
--     a = v    
--     result[#result+1] = {{tonumber(a),tonumber(b)}}
--     print("a:",result[1][1][1],"b:",result[1][1][2])
--     print("a",a,"b",b)

--     -- for i,v in ipairs(result) do
--     --   -- print(i, " note:",v[1][1][1],"amp:",v[1][1][2]) -- same as [1][1][1][1] [1][1][1][2] 
--     --   print(i)
--     -- end
--     end
--   end
--   -- print("result size", #result)
--   return result
-- end

function noteAmpArr(v)
  -- print("noteAmpArr(v)")
  result = {}
  if string.find(v,"~") then
    a,b = splitNoteAmp(v,"~")
    if b == nil then b = 1 end
    result[1] = {tonumber(a),tonumber(b)}
    -- result[2] = tonumber(b)
    -- print("a",result[1],"b",result[#result][1][2])
  else
    result[1] = {tonumber(v),tonumber(1)}
    -- result[2] = tonumber(1)
  end
  -- print("r a->",result[1][1],"r b->",result[1][2])
  return result
end

local counter = 0
function addToNoteTable(...)
  -- print("addToNoteTable(...)")
  
  result = {}
  local arg = {...}
  for i,v in ipairs(arg) do
    -- print(i)
    -- counter = counter+1
    -- local noteAmp = noteAmpArr(v)
    result[#result+1] = {noteAmpArr(v)}
    -- print("result["..i.."]={"..result[#result][1]..","..result[#result][2].."}")
    
    
  end
  -- for i,v in ipairs(result) do
  --   print(""..result[i][1]..","..result[i][2].."")
  -- end  
  -- print(#result)
  return result
end


function replaceRest()
  return 128 -- rest note value
end

function getValueForKey(t,key)
  for k,v in pairs(t) do
    if k == key then return v end
  end

  return nil
end

function noteCipherToMidi(c)
  ciphers = {c = 0,cb = 11,cs = 1,d = 2,db = 1,ds = 3,
              e = 4,eb = 3,f = 5,fb = 4,fs = 6,gb = 6,
              g = 7, gs = 8, ab = 8, a = 9, as = 10,
              bb = 10,b = 11, bs = 12}

  octave = string.sub(c,(#c > 1) and -1 or 1):match("^%-?%d+$")
  return getValueForKey(ciphers,string.sub(c,1,(octave ~= nil) and -2 or 1))+(12*((octave ~= nil) and octave or 0))
end

local note = lpeg.R("09")^1 --/toNoteAmp1 --/ tonumber
local sep = lpeg.S(" ")
local rest = lpeg.S("-") / replaceRest
-- local amp_chars = (lpeg.P("~.") + lpeg.P("~0.") + lpeg.P("~"))^1
-- local amp_char = lpeg.P("~")^1-- * lpeg.R("09"))^-1
-- local amp = ((lpeg.P("0.") + lpeg.P("."))^0 * lpeg.R("09"))^1
local note_amp = lpeg.P(note * (lpeg.S("~") * (lpeg.P("0.") + lpeg.S("."))^0 * lpeg.R("09"))^0)^1 / toNoteAmp
local cipher = lpeg.R("ag")
local oct = lpeg.R("08")^-1
local bs = lpeg.S("bs")^-1
local one_note_exclusions = lpeg.S(".(")^1
local cipher_bs_oct = lpeg.P(cipher * bs * oct)^1 / noteCipherToMidi

-- local noteP = lpeg.Cg(((note + cipher_bs_oct + rest)^1 * (sep^1 * (note + cipher_bs_oct + rest))^0)^1) / addToNoteTable
-- Working in v0.0.4
-- local noteP = lpeg.Cg((- one_note_exclusions * (note + cipher_bs_oct + rest)^1 * (sep^1 * (note + cipher_bs_oct + rest))^0)^1) / addToNoteTable 
-- local noteP = lpeg.Cg((- one_note_exclusions * ((note + cipher_bs_oct + rest)^1 * (amp_char * amp)^0) * ((sep^1 * (note + cipher_bs_oct + rest) * (amp_char * amp)^0))^0 )^1) / addToNoteTable
local noteP = lpeg.Cg((- one_note_exclusions * (note_amp + cipher_bs_oct + rest)^1 * (sep^1 * (note_amp + cipher_bs_oct + rest))^0)^1) / addToNoteTable
-- local noteAmpP = lpeg.Cg(note_amp)
local chordP = lpeg.Cg(("(" * ((note + cipher_bs_oct + rest)^1 * (sep^1 * (note + cipher_bs_oct + rest))^0)^1 * ")")) / addToChordTable
local subP = lpeg.Cg(("." * ((note + cipher_bs_oct + rest)^1 * (sep^1 * (note + cipher_bs_oct + rest))^0)^1 * ".")) / addToNoteTable

local V = lpeg.V
--[[local phraseG = lpeg.P {"phrase",
  phrase = lpeg.Ct(lpeg.Cg(((V"note" + V"chord" + V"sub")^1 * (sep^1 * (V"note" + V"chord" + V"sub"))^0)^1));
  chord = chordP;
  sub = subP;
  note = noteP;
} * -1]]

local phraseG = lpeg.P {"phrase",
  phrase = lpeg.Ct(lpeg.Cg(((V"note")^1 * (sep^1 * (V"note"))^0)^1));
  -- chord = chordP;
  -- sub = subP;
  note = noteP;
} * -1

--[[-- *testing*
-- r = phraseG:match("23~0.1 24~1 25~.234 26~1 27~.1")
-- r = phraseG:match("23 24~0.111 25 26")
-- r = phraseG:match("23 24 25 26 27 28 29")
-- r = phraseG:match("24")
-- print(counter)
-- print(#r)
-- print(r[1])
-- print(r[1][1])
-- print(r[1][1][1])
-- print(r[1][1][2])
-- print(r[1][1][1][1])
-- print(#r[1][2][1])
-- print(r[2][1][1][2])

-- print(r[2])
-- print(r[1][1][1][2])
-- print(r[2][1][1][1])
-- print(r[2][1][1][2])
-- print(r[3][1][1][1])
-- print(r[3][1][1][2])

-- print(r[2])
-- print(r[1][1][2][1])
-- print(r[2][1][1][2])

-- for i,v in ipairs(r) do
--   print(i, " note:",v[1][1][1],"amp:",v[1][1][2]) -- same as [1][1][1][1] [1][1][1][2] 
-- end]]