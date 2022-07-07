local lpeg = require 'lpeg'
lpeg.locale(lpeg) 

function addToChordTable(...)
  result = {}
  local arg = {...}
  for i,v in ipairs(arg) do
    result[#result+1] = v
  end
  return {result}
end

function addToNoteTable(...)
  result = {}
  local arg = {...}
  for i,v in ipairs(arg) do
    result[#result+1] = {v}
  end
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

local note = lpeg.R("09")^1 / tonumber
local sep = lpeg.S(" ")
local rest = lpeg.S("-") / replaceRest
local cipher = lpeg.R("ag")
local oct = lpeg.R("08")^-1
local bs = lpeg.S("bs")^-1
local cipher_bs_oct = lpeg.P(cipher * bs * oct)^1 / noteCipherToMidi

local noteP = lpeg.Cg(((note + cipher_bs_oct + rest)^1 * (sep^1 * (note + cipher_bs_oct + rest))^0)^1) / addToNoteTable
local chordP = lpeg.Cg(("(" * ((note + cipher_bs_oct + rest)^1 * (sep^1 * (note + cipher_bs_oct + rest))^0)^1 * ")")) / addToChordTable
local subP = lpeg.Cg(("." * ((note + cipher_bs_oct + rest)^1 * (sep^1 * (note + cipher_bs_oct + rest))^0)^1 * ".")) / addToNoteTable

local V = lpeg.V
local linePhrase = lpeg.P {"phrase",
  phrase = lpeg.Ct(lpeg.Cg(((V"note" + V"chord" + V"sub")^1 * (sep^1 * (V"note" + V"chord" + V"sub"))^0)^1));
  chord = chordP;
  sub = subP;
  note = noteP;
}