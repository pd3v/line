-- find lpeg library
package.cpath = 'externals/?/lib?.dylib;externals/?/lib?.so;../lib/liblpeg.dylib;../lib/liblpeg.so;/usr/local/lib/lib?.dylib;/usr/local/lib/lib?.so' .. package.cpath

local lpeg = require 'lpeg'
local write = io.write
lpeg.locale(lpeg)

local AMP_SYMBOL = "~"
local NO_MATCH = {{'e', {{255,0}}}}
local ampGlobal = 127
local range_min,range_max = 0,127

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
    result = {tonumber(note),tonumber(amp*127)}
  else
    result =  {tonumber(v),127}
  end

  return result
end

-- :FIX runs for every note in chord. should run once per chord.
function toChordAmp(v)
  if string.find(v,AMP_SYMBOL) then
    chord,amp = splitNoteAmp(v,AMP_SYMBOL)
    amp = amp*127
  else
    amp = 127
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

function noteCipherToMidi(cipher)
  local note, amp, octave
  ciphers = { c = 0,cf = 11,cs = 1,d = 2,df = 1,ds = 3,
              e = 4,ef = 3,f = 5,ff = 4,fs = 6,gf = 6,
              g = 7,gs = 8,af = 8,a = 9,as = 10,bf = 10,b = 11
            }
  if string.find(cipher,AMP_SYMBOL) then
    note,amp = splitNoteAmp(cipher,AMP_SYMBOL)
    octave = note:match("%d+") ~= nil and note:match("%d+") or 0
    note = note:match("%a+")
  else
    note = cipher:match("%a+")
    octave = cipher:match("%d+") ~= nil and cipher:match("%d+") or 0
    amp = 1
  end

  if getValueForKey(ciphers,note) then
    cipher_to_midi = getValueForKey(ciphers, note) + (12 * octave)
  else
    return {0,0} -- cipher not matching error
  end

  return {cipher_to_midi, amp * 127}
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
local chordP = lpeg.Cg(("(" * ((note_amp + cipher_bs_oct_amp + rest)^1 * (sep^1 * (note_amp + cipher_bs_oct_amp + rest))^0)^1 * ")" * chord_amp)) / addToChordTable
local subP = lpeg.Cg(("." * ((noteP + chordP + rest)^1 * (sep^1 * (noteP + chordP + rest))^0)^1 * ".")) / addToSubTable

local V = lpeg.V
local phraseG = lpeg.P {"phrase",
  phrase = lpeg.Ct(lpeg.Cg(((V"note" + V"chord" + V"sub")^1 * (sep^1 * (V"note" + V"chord" + V"sub"))^0)^1));
  chord = chordP;
  sub = subP;
  note = noteP;
} * -1

-- Phrases of different ranges other then MIDI 0-127
function rescaleToMIDI(v)
  v = (v-range_min)*127/(range_max-range_min)
  return math.ceil(v)
end

local numbr = lpeg.P(lpeg.P(("0.") + lpeg.S("."))^0 * note)^1 / rescaleToMIDI
local rangeP = lpeg.Cg((numbr)^1 * (sep^1 * numbr)^0)

local rangeG = lpeg.P {"prange",
  prange = lpeg.Ct(lpeg.Cg(((V"range")^1 * (sep^1 * V"range")^0)^1));
  range = rangeP;
} * -1
