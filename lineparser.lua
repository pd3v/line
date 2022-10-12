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

function addToNoteTable(...)
  result = {}
  local arg = {...}
  
  for i,v in ipairs(arg) do
    result[#result+1] = v
  end

  return {'n',result}
end

function addToSubTable(...)
  result = {}
  local arg = {...}
  
  for i,v in ipairs(arg) do
    result[#result+1] = v
  end

  return {'s',result}
end

function addToChordTable(...)
  result = {}
  local arg = {...}

  for i,v in ipairs(arg) do
    v[2] = ampGlobal
    result[#result+1] = v
  end

  return {'c',result}
end

function toNoteAmp(v)
  result = {}
  if string.find(v,AMP_SYMBOL) then
    note,amp = splitNoteAmp(v,AMP_SYMBOL)
    result = {tonumber(note),tonumber(amp)}
  else
    result =  {tonumber(v),tonumber(1)}
  end
  return result
end

-- :FIX runs for every note in chord. should run one per chord.
function toChordAmp(v)
  if string.find(v,AMP_SYMBOL) then
    chord,amp = splitNoteAmp(v,AMP_SYMBOL)
  else
    amp = 1  
  end
  ampGlobal = amp
end

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
  local note,amp
  ciphers = { c = 0,cb = 11,cs = 1,d = 2,db = 1,ds = 3,
              e = 4,eb = 3,f = 5,fb = 4,fs = 6,gb = 6,
              g = 7, gs = 8, ab = 8, a = 9, as = 10,
              bb = 10,b = 11
            }
  
  if string.find(c,AMP_SYMBOL) then
    note,amp = splitNoteAmp(c,AMP_SYMBOL)
  else
    note = c
    amp = 1
  end

  octave = string.sub(note,(#note > 1) and -1 or 1):match("^%-?%d+$")
  cipher_to_midi = getValueForKey(ciphers,string.sub(note,1,(octave ~= nil) and -2 or 1))+(12*((octave ~= nil) and octave or 0))

  return toNoteAmp(tostring(cipher_to_midi)..AMP_SYMBOL..tostring(amp))
end

local note = lpeg.R("09")^1
local amp = note
local sep = lpeg.S(" ")
local rest = lpeg.S("-") / replaceRest

local note_amp = lpeg.P(note * (lpeg.S(AMP_SYMBOL) * (lpeg.P("0.") + lpeg.S("."))^0 * amp)^0)^1 / toNoteAmp
local chord_amp = lpeg.P((lpeg.S(AMP_SYMBOL) * (lpeg.P("0.") + lpeg.S("."))^0 * amp))^0 / toChordAmp
local cipher = lpeg.R("ag")
local oct = lpeg.R("08")^-1
local bs = lpeg.S("bs")^-1
local one_note_exclusions = lpeg.S(".(")^1
local cipher_bs_oct = lpeg.P(cipher * bs * oct)^1 / noteCipherToMidi
local cipher_bs_oct_amp = lpeg.P((cipher * bs * oct) * (lpeg.S(AMP_SYMBOL) * (lpeg.P("0.") + lpeg.S("."))^0 * amp)^0)^1 / noteCipherToMidi

local noteP = lpeg.Cg((- one_note_exclusions * (note_amp + cipher_bs_oct_amp + rest)^1 * (sep^1 * (note_amp + cipher_bs_oct_amp + rest))^0)^1) / addToNoteTable
local subP = lpeg.Cg(("." * ((note_amp + cipher_bs_oct_amp + rest)^1 * (sep^1 * (note_amp + cipher_bs_oct_amp + rest))^0)^1 * ".")) / addToSubTable
local chordP = lpeg.Cg(("(" * ((note_amp + cipher_bs_oct_amp + rest)^1 * (sep^1 * (note_amp + cipher_bs_oct_amp + rest))^0)^1 * ")" * chord_amp)) / addToChordTable

local V = lpeg.V
local phraseG = lpeg.P {"phrase",
  phrase = lpeg.Ct(lpeg.Cg(((V"note" + V"chord" + V"sub")^1 * (sep^1 * (V"note" + V"chord" + V"sub"))^0)^1));
  chord = chordP;
  sub = subP;
  note = noteP;
} * -1
