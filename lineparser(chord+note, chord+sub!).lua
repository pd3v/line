local lpeg = require 'lpeg'
local write = io.write
lpeg.locale(lpeg)

local AMP_SYMBOL = "~"
local ampGlobal

function splitNoteAmp(s,sep)
  sep = lpeg.P(sep)
  local elem = lpeg.C((1 - sep)^0)
  local p = elem * (sep * elem)^0
  return lpeg.match(p, s)
end

--[[
function addToChordTable(...)
  result = {}
  local arg = {...}
  for i,v in ipairs(arg) do
    result[#result+1] = v
  end
  return {result}
end
]]

function addToChordTable(...)
  print("__chord__")
  result = {}
  local arg = {...}
  local cnt = 1
  -- print(toChordAmp(...))
  -- splitNoteAmp(arg,AMP_SYMBOL)
  write("{{ ")
  for i,v in ipairs(arg) do
    -- print(toChordAmp(v))
    v[2] = ampGlobal
    -- print("v:",v[2],ampGlobal)
    result[#result+1] = v
    write(""..v[1].."")
    if cnt < #arg then write(",") end
    cnt = cnt + 1
  end
  write(" }}\n")
  return {result}
  -- return result
end

function addToSubTable(...)
  print("__sub__")
  result = {}
  local arg = {...}
  local cnt = 1
  write("{ ")
  for i,v in ipairs(arg) do
    result[#result+1] = {v}
   write("{"..v[1].."}")
   if cnt < #arg then write(",") end
   cnt = cnt+1
  end
  write(" }\n")
  return result
end

--[[
function addToNoteTable(...)
  print("__note__")
  result = {}
  local arg = {...}
  local cnt = 1
  write("{ ")
  for i,v in ipairs(arg) do
    result[#result+1] = {v}
   write("{"..v[1].."}")
   if cnt < #arg then write(",") end
   cnt = cnt+1
  end
  write(" }\n")
  return result
end
]]

function addToNoteTable(...)
  print("__note__")
  result = {}
  local arg = {...}
  local cnt = 1
  -- if #arg == 1 then write ("note") else write ("sub") end
  write("{ ")
  for i,v in ipairs(arg) do
    for _i,_v in ipairs(v) do
      result[#result+1] = _v
      write("{{".._v[1].."}}")
      if cnt < #arg then write(",") end
      cnt = cnt+1
    end
  end
  write(" }\n")
  return result
end

function toNoteAmp(v)
  write("_toNoteAmp_")
  result = {}
  if string.find(v,AMP_SYMBOL) then
    note,amp = splitNoteAmp(v,AMP_SYMBOL)
    -- if amp == nil then amp = 1 end
    result = {tonumber(note),tonumber(amp)}
  else
    result =  {tonumber(v),tonumber(1)}
  end
  write("{"..result[1].."}\n")
  return {result}
end

function toChordNoteAmp(v)
  write("_toChordNoteAmp_")
  result = {}
  if string.find(v,AMP_SYMBOL) then
    note,amp = splitNoteAmp(v,AMP_SYMBOL)
    -- if amp == nil then amp = 1 end
    result = {tonumber(note),tonumber(amp)}
  else
    result =  {tonumber(v),tonumber(1)}
  end
  write("{"..result[1].."}\n")
  return result
end


function toSubNoteAmp(v)
  print("_toSubNoteAmp_")
  result = {}
  if string.find(v,AMP_SYMBOL) then
    note,amp = splitNoteAmp(v,AMP_SYMBOL)
    -- if amp == nil then amp = 1 end
    result = {tonumber(note),tonumber(amp)}
  else
    result =  {tonumber(v),tonumber(1)}
  end
  return result
end


-- :FIX runs for every note in chord. should run one per chord.
function chordAmp(v)
  -- result = {}
  -- if string.find(v,AMP_SYMBOL) then
  --   note,amp = splitNoteAmp(v,AMP_SYMBOL)
  --   -- if amp == nil then amp = 1 end
  --   result = {tonumber(note),tonumber(amp)}
  -- else
  --   result =  {tonumber(v),tonumber(1)}
  -- end
  -- return result
  -- for i,v in ipairs(arg) do
  --   print("--")
  --   print(i,v)
  -- end
  -- print("chordAmp")
  -- print(v)
  if string.find(v,AMP_SYMBOL) then
    chord,amp = splitNoteAmp(v,AMP_SYMBOL)
  --   print("amp"..amp.."")
    -- if amp == nil then amp = 1 end
  else
    amp = 1  
  end
  -- print("value:"..v.." chord:"..chord.." amp:"..amp.."")
  -- print("value:"..v.." amp:"..amp.."")
  -- return amp
  ampGlobal = amp
end


-- function addToNoteTable(...)
--   result = {}
--   local arg = {...}
--   for i,v in ipairs(arg) do
--     -- print(v[1][1],v[1][2])
--     result[#result+1] = {v}
--   end
--   return result
-- end


function replaceRest()
  return {128,0} -- rest note value
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

  local note,amp
  if string.find(c,AMP_SYMBOL) then
    note,amp = splitNoteAmp(c,AMP_SYMBOL)
  else
    note = c
    amp = 1
  end

  -- octave = string.sub(c,(#c > 1) and -1 or 1):match("^%-?%d+$")
  octave = string.sub(note,(#note > 1) and -1 or 1):match("^%-?%d+$")
  -- return getValueForKey(ciphers,string.sub(note,1,(octave ~= nil) and -2 or 1))+(12*((octave ~= nil) and octave or 0))

  -- local eh = getValueForKey(ciphers,string.sub(note,1,(octave ~= nil) and -2 or 1))+(12*((octave ~= nil) and octave or 0))
  -- local ok = toNoteAmp(tostring(getValueForKey(ciphers,string.sub(note,1,(octave ~= nil) and -2 or 1))+(12*((octave ~= nil) and octave or 0)))..AMP_SYMBOL..tostring(amp))
  -- print("note_amp",ok[1],ok[2])

  return toNoteAmp(tostring(getValueForKey(ciphers,string.sub(note,1,(octave ~= nil) and -2 or 1))+(12*((octave ~= nil) and octave or 0)))..AMP_SYMBOL..tostring(amp))
  -- return 24
end

local note = lpeg.R("09")^1 --/toNoteAmp1 --/ tonumber
local amp = note
local sep = lpeg.S(" ")
local rest = lpeg.S("-") / replaceRest

-- local amp_char = lpeg.P(AMP_SYMBOL)^1-- * lpeg.R("09"))^-1
-- local amp = ((lpeg.P("0.") + lpeg.P("."))^0 * lpeg.R("09"))^1
local note_amp = lpeg.P(note * (lpeg.S(AMP_SYMBOL) * (lpeg.P("0.") + lpeg.S("."))^0 * amp)^0)^1 / toNoteAmp
local sub_note_amp = lpeg.P(note * (lpeg.S(AMP_SYMBOL) * (lpeg.P("0.") + lpeg.S("."))^0 * amp)^0)^1 / toSubNoteAmp
-- local chord_amp = lpeg.P((lpeg.S(AMP_SYMBOL) * (lpeg.P("0.") + lpeg.S("."))^0 * amp)^0)^1 / chordAmp
local chord_amp = lpeg.P((lpeg.S(AMP_SYMBOL) * (lpeg.P("0.") + lpeg.S("."))^0 * amp))^0 / toChordNoteAmp
local cipher = lpeg.R("ag")
local oct = lpeg.R("08")^-1
local bs = lpeg.S("bs")^-1
local one_note_exclusions = lpeg.S(".(")^1
local cipher_bs_oct = lpeg.P(cipher * bs * oct)^1 / noteCipherToMidi
local cipher_bs_oct_amp = lpeg.P((cipher * bs * oct) * (lpeg.S(AMP_SYMBOL) * (lpeg.P("0.") + lpeg.S("."))^0 * amp)^0)^1 / noteCipherToMidi

-- local noteP = lpeg.Cg(((note + cipher_bs_oct + rest)^1 * (sep^1 * (note + cipher_bs_oct + rest))^0)^1) / addToNoteTable
-- Working in v0.0.4
-- local noteP = lpeg.Cg((- one_note_exclusions * (note + cipher_bs_oct + rest)^1 * (sep^1 * (note + cipher_bs_oct + rest))^0)^1) / addToNoteTable 
-- local noteP = lpeg.Cg((- one_note_exclusions * ((note + cipher_bs_oct + rest)^1 * (amp_char * amp)^0) * ((sep^1 * (note + cipher_bs_oct + rest) * (amp_char * amp)^0))^0 )^1) / addToNoteTable
local noteP = lpeg.Cg((- one_note_exclusions * (note_amp + cipher_bs_oct_amp + rest)^1 * (sep^1 * (note_amp + cipher_bs_oct_amp + rest))^0)^1) / addToNoteTable
local chordP = lpeg.Cg(("(" * ((note_amp + cipher_bs_oct_amp + rest)^1 * (sep^1 * (note_amp + cipher_bs_oct_amp + rest))^0)^1 * ")" * chord_amp)) / addToChordTable
local subP = lpeg.Cg(("." * ((sub_note_amp + cipher_bs_oct_amp + rest)^1 * (sep^1 * (sub_note_amp + cipher_bs_oct_amp + rest))^0)^1 * ".")) / addToSubTable --addToNoteTable

local V = lpeg.V
local phraseG = lpeg.P {"phrase",
  phrase = lpeg.Ct(lpeg.Cg(((V"note" + V"chord" + V"sub")^1 * (sep^1 * (V"note" + V"chord" + V"sub"))^0)^1));
  chord = chordP;
  sub = subP;
  note = noteP;
} * -1

--[[
-- local phraseG = lpeg.P {"phrase",
--   phrase = lpeg.Ct(lpeg.Cg(((V"note")^1 * (sep^1 * (V"note"))^0)^1));
--   -- chord = chordP;
--   -- sub = subP;
--   note = noteP;
-- } * -1
-- local phraseG = lpeg.P {"phrase",
--   phrase = lpeg.Ct(lpeg.Cg(((V"note")^1 * (sep^1 * (V"note"))^0)^1));
--   -- chord = chordP;
--   -- sub = subP;
--   note = noteP;
-- } * -1
-- *testing*
]]
-- r =  phraseG:match("23~0.1 24 25 26~1 27~.1")
r =  phraseG:match("(23 24 25)~.5")
-- r =  phraseG:match("(23 24 25)~.5 26 27 28")
--[[
  -- r =  phraseG:match("(23 24 25)~.5 26 27")
-- print(r)
-- print(r[1])
-- print(r[1][1][1])
-- print(r[1][1][1][1])
-- print(r[1][1][2][1])
-- print(r[1][1][1][1])
-- print(r[1][2][1][1])
-- print(r[3][1][1][1])
-- print(r[1])

-- print(r[1][6][1][2])

-- r = phraseG:match("23~0.1 24 25 26~1 27~.1")
-- r = phraseG:match("cs4~.1 c3 24 25 26~1 f4~.5")
-- r = phraseG:match("c2~0.1 c4 cs4 d4~1 d5~.1")
-- r = phraseG:match("23 24~0.111 25 26~.1234")
-- r = phraseG:match("23 24 25 26 27 28 29")
-- -- r = phraseG:match("24")
-- print(r[1][3][1][1])  --  {{{},{},{},{26,1},{},{},{}}}
-- print(r[1][3][1][2])  --  
-- print(r)
-- for i,v in ipairs(r[1]) do
--   print(i,v[1][1],v[1][2])
-- end  
-- print(r[1][6][1][1])
-- print(r[1][6][1][2])
]]