/*!
  @file    ud5_msq.cpp
  @version 0.998
  @brief   Synthesizer by wavetable and music sequencer by simple MML
  @date    2024/9/29
  @author  T.Uemitsu

  @copyright
    Copyright (c) BestTechnology CO.,LTD. 2024
    All rights reserved.
 */

#include  "ud5.h"
#include  "fixedPoint.h"
#include  <string.h>
#include  <ctype.h>

#define _MAX_NOTES            (6)
#define _MAX_TONE             (10)
//#define _MAX_MUSICSQ          (6)
#define _UPDATE_FREQ          (11025)
//#define _DEFAULT_MUSICICSIZE  (700)

//starage for ADSR envelope
static int8_t adsr[_MAX_MUSICSQ][4];
//interpolated signed wave samples
static int8_t wave[_MAX_TONE][256] = { 0 };
//detune
static int8_t detune[_MAX_MUSICSQ] = { 0 };
//selected wave no
static uint8_t waveno[_MAX_MUSICSQ] = { 0 };

static int8_t noteMap[16][128];


//! Set envelope
// v[0]:Attack, v[1]:Decay, v[2]:Sustain, v[3]Release
// range:0..127
void SetEnvelope (uint8_t ch, const int8_t *v) {
  if (ch + 1 > _MAX_MUSICSQ) return;
  for (int i = 0; i < 4; i++) adsr[ch][i] = v[i];
}

//! Set wave form
// v:-128..127
void SetWaveForm (uint8_t no, const int8_t *v, uint8_t size) {
  if (no + 1 > _MAX_TONE || size < 0 || size > 256) return;
  for (int t = 0; t < 256; t++) wave[no][t] = v[(t * (int)size) / 256];
}

//! Frequency table
// this frequency table is for 44100Hz
struct CfreqIncTab {
  uint32_t ary[128];
  constexpr CfreqIncTab () : ary () {
    for (int i = 0; i < 128; i++) ary[i] = UINT32_MAX / 44100.0 * (440.0 * __builtin_pow (2.0, (i - 69) / 12.0));
  }
};
static constexpr CfreqIncTab freqIncTab;

//~reciprocal look up table
struct Crcp {
  int16_t ary[128];
  constexpr Crcp () : ary () {
    for (int i = 0; i < 128; i++) ary[i] = UINT16_MAX / (i + 1) - 1;
  }
};
static constexpr Crcp rcp;

/*!
 @brief Wavetable synthesizer class
 @note
   The classifieds are based on <a href="https://github.com/bitluni/arduinoMIDISynth">this repository</a>.
 */
struct Note {
  static bool use_notemap;  // normally set to false because the steady-state deviation of the reverberation remains
  uint8_t channel;
  uint8_t note;
  int8_t state;
  // fixed point precalculated parameters
  uf8p24 phase;
  uf8p24 freqInc;
  uf8p24 envelopePhase;
  uf0p16 envelopePhaseIncA;
  uf0p16 envelopePhaseIncD;
  uf0p16 envelopePhaseIncR;
  uf0p8 envelopeAmpS;
  uf0p8 envelopeAmpR;
  uf0p8 envelopeAmp;
  uf0p8 envelopeAmpDeltaS;
  uf0p8 envelopeAmpDeltaA;
  uf0p8 velocity;
  uf0p8 envelopeAmpA;

  Note() {
    state = -1;
    envelopeAmp = 0;
  }

  //! set envelope
  // v:0..127
  void setEnvelope (uint8_t v) {
    if (channel + 1 > _MAX_MUSICSQ) return;
    velocity = (v << 1) + 1;     // scale velocity to [1 - 255]
    // precalculate values
    // each envelope phase counts for 65536 steps (16 bit). the actual time they take is resulting from the increment we count up to thes value
    // the upper 8 bits of the envelope phase indicate in which region of ADSR we are
    envelopePhase = 0;
    envelopePhaseIncA = (rcp.ary[adsr[channel][0]]);                              // 1/A
    envelopePhaseIncD = (rcp.ary[adsr[channel][1]]);                              // 1/D
    envelopePhaseIncR = (rcp.ary[adsr[channel][3]]);                              // 1/R
    envelopeAmpS = mul_uf0p8_uf0p8_uf0p8 (velocity, (adsr[channel][2] << 1) + 1); // S (scaled to 1 - 255) * velocity
    envelopeAmpDeltaS = velocity - envelopeAmpS;       // delta between max amp
    envelopeAmpDeltaA = velocity - envelopeAmpA;       // delta from 0 or last amplitude if note is replayed
  }

  void replay (uint8_t v) {
    envelopeAmpA = envelopeAmp; // start from current amplitude to avoid clicks
    setEnvelope (v);     // write new parameters
  }

  //! tone on
  void on (uint8_t ch, uint8_t n, uint8_t v, bool slur) {
    const int8_t detune_table[31] = {-56,-52,-48,-44,-41,-37,-33,-29,-26,-22,-18,-15,-11,-7,-4,0,4,7,11,15,18,22,26,29,33,37,41,44,48,52,56};  //center=15 (1/16step 1/1000の差分のみ)
    // set note parameters
    channel = ch;
    note = n;
    if (!slur) phase = 0;
    freqInc = freqIncTab.ary[n] + (freqIncTab.ary[n] / 1000) * (detune_table[detune[ch]]);
    freqInc <<= 2;      // have to quadruple the increment because of the we went down to a qarter of the frequency (44.1kHz->11.025kHz)
    if (!slur) {
      envelopeAmpA = 0;   // no playing atm start attack from 0
      setEnvelope (v);
      state = 0;          // set statte to plaing
    } else {
      velocity = (v << 1) + 1;
      envelopeAmpS = mul_uf0p8_uf0p8_uf0p8(velocity, (adsr[channel][2] << 1) + 1);
      envelopeAmpDeltaS = velocity - envelopeAmpS;
      envelopeAmpDeltaA = velocity - envelopeAmpA;
    }
  }

  //! tone off
  void off (void) {
    envelopeAmpR = envelopeAmp; // save release amplitude
    if(envelopeAmpR)
      envelopePhase = 0x3000000;   // goto release
    else
      envelopePhase = 0x4000000;   // no relase phase
  }

  //! mixer
  int8_t mix (void) {
    switch(cast_uf8p24_uf8p0 (envelopePhase)) {  //upper 8bits of envelope pahse are taking to identify the phase
      case 0:
        envelopeAmp = envelopeAmpA + mul_uf0p8_uf0p8_uf0p8(cast_uf8p24_uf0p8(envelopePhase), envelopeAmpDeltaA);  //ramping up during Attack pahse to velocity amp
        envelopePhase += envelopePhaseIncA;  //adding the increment. the higher the inc the shorter the attack period
        break;
      case 1:
        envelopeAmp = velocity - mul_uf0p8_uf0p8_uf0p8(cast_uf8p24_uf0p8(envelopePhase), envelopeAmpDeltaS);  //ramping decay down to sustain level
        envelopePhase += envelopePhaseIncD;  //same procedure her as in attack
        break;
      case 2:
        if(!envelopeAmpS) { //if sustain level is zerowe are done here, go to end
          envelopePhase = 0x4000000;
          return 0;
        }
        envelopeAmp = envelopeAmpS;      //set the amplitude to calculated value othervise
        break;
      case 3:
        envelopeAmp = envelopeAmpR - mul_uf0p8_uf0p8_uf0p8(cast_uf8p24_uf0p8(envelopePhase), envelopeAmpR);  //ramp down the last aplitude we saved to zero during the relese pahse 
        envelopePhase += envelopePhaseIncR;  //same as above
        break;
      case 4:
        state = -1;              //done with the note, clear the states
        if (use_notemap) noteMap[channel][note] = -1;
        return 0;
    }
    f8p0 s = 0;
    s = wave[waveno[channel]][cast_uf8p24_uf8p0(phase)];  //read the waveform from the table taking the high 8 bits of the time phase
    s = mul_f8p0_uf0p8_f8p0 (s, envelopeAmp);  //mutliplying the wave by the amplutide calculated from the ADSR and velocity above
    phase += freqInc;  //increment the pahase of the wave freqency 
    return s;
  }
};

//! The number of polyphony is determined by _MAX_NOTES
static Note notes[_MAX_NOTES];

//! Interrupt callback from CDAC
static int32_t dac_note_cb(void) {
  int32_t v = 0;
  for (int8_t i = 0; i < _MAX_NOTES; i++)
    if (notes[i].state >= 0) v += notes[i].mix ();
  return v;
}

static CPCM *ppcm = NULL;

// Whether to use notemap (false is highly recommended)
bool Note::use_notemap = false;

  // Store conversion data
  bool CTinyMusicSq::store_MI (int cmd, int param) {
    if (mi_ind + 2 > size) return false;
    IC[mi_ind].bit.H = cmd;
    IC[mi_ind].bit.L = param;
    IC[++mi_ind].byte = 0;
    return true;
  }
  bool CTinyMusicSq::store_MI_b (uint8_t d) {
    store_MI (d & 0xf, 0xf);
    return store_MI (d >> 4, 0xf);
  }

  // Convert note length
  int CTinyMusicSq::len2ind (int ll) {
    switch (ll) {
      case 1:   return 0;  // whole note
      case 2:   return 1;  // 1/2
      case 3:   return 2;  // 1/3
      case 4:   return 3;  // 1/4
      case 6:   return 4;  // 1/6
      case 8:   return 5;  // 1/8
      case 12:  return 6;  // 1/12
      case 16:  return 7;  // 1/16
      case 24:  return 8;  // 1/24
      case 32:  return 9;  // 1/32
      case 48:  return 10; // 1/48
      case 64:  return 11; // 1/64
      case 25:  return 12; // 1/2.5
      case 45:  return 13; // 1/4.5
      case 85:  return 14; // 1/8.5
      case 165: return 15; // 1/16.5
    }
    return -1;
  }

  // scale analysis
  bool CTinyMusicSq::analyze_scale (const char **MML, int oct, int tnum, int len) {
    bool result = false;
    char c;
    int n = len;
    // accidental
    if ((c = *++*MML) != 0) {
      if ((c == '#') || (c == '+')) {
        tnum++;
        (*MML)++;
      } else if (c == '-') {
        tnum--;
        (*MML)++;
      }
    }
    // length
    if ((c = **MML) != 0) if ((('0' <= c) && (c <= '9')) || (c == '.')) {
      if (c == '.') n = n * 10 + 5;
      else n = c - '0';
      if ((c = *++*MML) != 0) if ((('0' <= c) && (c <= '9')) || (c == '.')) {
        if (c == '.') n = n * 10 + 5;
        else n = n * 10 + (c - '0');
        if ((c = *++*MML) != 0) if (c == '.') {
          n = n * 10 + 5;
          (*MML)++;
        }
      }
    }
    if (tnum < 2) {  // c-
      store_MI (15, oct - 1);
      store_MI (13, len2ind (((len2ind (n) != -1) ? n : len)));
      result = store_MI (15, oct);
    } else if (tnum > 13) {  // b+
      store_MI (15, oct + 1);
      store_MI (2, len2ind (((len2ind (n) != -1) ? n : len)));
      result = store_MI (15, oct);
    } else {  // c..b
      result = store_MI (tnum, len2ind (((len2ind (n) != -1) ? n : len)));
    }
    return result;
  }

  void CTinyMusicSq::noteOff (uint8_t ch, uint8_t n) {
    if (Note::use_notemap) {
      if (noteMap[ch][n] >= 0) notes[noteMap[ch][n]].off ();
    } else {
      notes[ch].off ();
    }
  }

  void CTinyMusicSq::noteOn (uint8_t ch, uint8_t n, uint8_t v, bool slur) {
    if (!v) noteOff (ch, n);
    else {
      if (Note::use_notemap) {
        if (noteMap[ch][n] >= 0) {
          notes[noteMap[ch][n]].replay (v);
        } else {
          for (uint8_t i = 0; i < _MAX_NOTES; i++) {
            if(notes[i].state == -1) {
              notes[i].on (ch, n, v, slur);
              noteMap[ch][n] = i;
              return;
            }
          }
        }
      } else {
        notes[ch].on (ch, n, v, slur);
      }
    }
  }

  void CTinyMusicSq::noteReset (uint8_t ch) {
    if (Note::use_notemap) memset (&noteMap[ch], -1, 128);
    notes[ch].envelopeAmp = 0;
    notes[ch].state = -1;
  }

  // play task
  void CTinyMusicSq::playtask (void) {
    stat = stStopping;
    vTaskSuspend (NULL);
    while (1) {
      class CLIFO {
      private:
        int8_t buff[16];
        int8_t top;
      public:
        void push (int8_t cnt) {
          if (top <= 15) buff[top++] = cnt;
        }
        int8_t pop (void) {
          if (top >= 0) return buff[--top]; else return -1;
        }
        CLIFO () :top (0) {}
      } repnum;
      //                               1    2   3   4   6   8   12  16  24 32 48 64 2.   4.  8.  16.
      const uint8_t NoteLength[16] = { 192, 96, 64, 48, 32, 24, 16, 12, 8, 6, 4, 3, 144, 72, 36, 18,};
      bool skip = true, slur = false, brk = false;
      uint8_t
        pcmNo = 0,   // pcm table no
        velo = 63,   // velocity
        oct = 4,     // octave
        hierarchy,   // hierarchy of iterations
        _adsr[4] = {0,32,32,0};    // envelope
      int16_t
        oldtone = -1,
        tickcnt = 0,    // tone length counter
        sqind = 0;      // progress
      uint32_t tempo = 1 * ((10 - /*t*/5) * 1 + 4);

      stat = stPlaying;

      noteReset (ChannelNo);
      waveno[ChannelNo] = 0;
      detune[ChannelNo] = 15;
      SetEnvelope (ChannelNo, (int8_t *)_adsr);

      uint32_t tick = xTaskGetTickCount (), ad = 0;
      for (;;) {
        if (--tickcnt <= 0) {
          do {
            switch (IC[sqind].bit.H) {
              case 0:
                switch (IC[sqind++].bit.L) {
                  case 0:   // fin
                    if (!enable_pcm) {
                      if (oldtone != -1) noteOff (ChannelNo, oldtone);
                    }
                    tempo = 0;
                    skip = false;
                    brk = true;
                    break;
                  case 1:   // slur
                    slur = true;
                    skip = true;
                    break;
                  case 2:   // repetition head (infinite)
                    repnum.push (99);
                    skip = true;
                    break;
                  case 3:   // repetition head (2)
                    repnum.push (1);
                    skip = true;
                    break;
                  case 4:   // repetition head (3)
                    repnum.push (2);
                    skip = true;
                    break;
                  case 5:   // repetition head (4)
                    repnum.push (3);
                    skip = true;
                    break;
                  case 6:   // repetition head (5)
                    repnum.push (4);
                    skip = true;
                    break;
                  case 7:   // repetition head (6)
                    repnum.push (5);
                    skip = true;
                    break;
                  case 8:   // repetition head (7)
                    repnum.push (6);
                    skip = true;
                    break;
                  case 9:   // repetition head (8)
                    repnum.push (7);
                    skip = true;
                    break;
                  case 10:  // repetition head (9)
                    repnum.push (8);
                    skip = true;
                    break;
                  case 11:{ // repetition tail
                    skip = true;
                    hierarchy = 1;
                    int8_t n = repnum.pop ();
                    if (n > 0) {
                      if (n > 8) repnum.push (99);
                      else repnum.push (--n);
                      while (--sqind >= 0) {
                        uint8_t b = IC[sqind].byte;
                        if (b == 0x0b) {  // ']'
                          hierarchy++;
                        } else if ((0x02 <= b) && (b <= 0x0a)) {
                          if (--hierarchy == 1) {
                            sqind++;
                            break;
                          }
                        }
                      }
                    }
                    break; }
                  case 12:  // velocity
                    velo = IC[sqind++].bit.H & 0xf;
                    velo |= IC[sqind++].bit.H << 4;
                    skip = true;
                    break;
                  case 13:  // wave table
                    if (enable_pcm) {
                      pcmNo = IC[sqind++].bit.H;
                    } else {
                      if (!slur) {
                        if (oldtone != -1) noteOff (ChannelNo, oldtone);
                        noteReset (ChannelNo);
                      }
                      waveno[ChannelNo] = IC[sqind++].bit.H;
                      SetEnvelope (ChannelNo, (int8_t *)_adsr);
                    }
                    skip = true;
                    break;
                  case 14:  // detune
                    detune[ChannelNo] = IC[sqind++].bit.H + 7;
                    skip = true;
                    break;
                  case 15:
                    for (int i = 0; i < 4; i++) {
                      _adsr[i] = IC[sqind++].bit.H & 0xf;
                      _adsr[i] |= IC[sqind++].bit.H << 4;
                    }
                    SetEnvelope (ChannelNo, (int8_t *)_adsr);
                    skip = true;
                    break;
                }
                break;
              case 1:   // rest
                tickcnt = NoteLength[IC[sqind++].bit.L];
                if (!enable_pcm) {
                  if (oldtone != -1 && !slur) noteOff (ChannelNo, oldtone);
                }
                slur = false;
                skip = false;
                break;
              case 2:   // C
              case 3:   // C+
              case 4:   // D
              case 5:   // D+
              case 6:   // E
              case 7:   // F
              case 8:   // F+
              case 9:   // G
              case 10:  // G+
              case 11:  // A
              case 12:  // A+
              case 13:  // B
                if (enable_pcm){
                  ppcm->play (pcmNo, oct * 12 + IC[sqind].bit.H - 2, velo);

                  tickcnt = NoteLength[IC[sqind++].bit.L];
                } else {
                  if (!slur && oldtone != -1) noteOff (ChannelNo, oldtone);
                  tickcnt = NoteLength[IC[sqind].bit.L];
                  oldtone = (oct + 2) * 12 + IC[sqind++].bit.H - 2;
                  noteOn (ChannelNo, oldtone, velo, slur);
                }
                slur = false;
                skip = false;
                break;
              case 14:  // tempo
                tempo = 1 * ((10 - IC[sqind++].bit.L) * 1 + 4);
                skip = true;
                break;
              case 15:  // octave
                oct = IC[sqind++].bit.L;
                skip = true;
                break;
            }
            if (stat == stStopExec) {
              if (!enable_pcm) {
                if (oldtone != -1) noteOff (ChannelNo, oldtone);
              }
              skip = false;
              brk = true;
            }
          } while (skip);
        } //if
        if (brk) break;
        // Use the note length for delay time
#if 1
        while (((tick + ad + tempo) > xTaskGetTickCount ()) && (stat == stPlaying)) vTaskDelay (1);
        ad += tempo;
#else
        vTaskDelay (tempo);
#endif
      } //for
      stat = stStopping;
      vTaskSuspend (NULL);
    }
  }

  //! convert MML to intermediate code
  bool CTinyMusicSq::Convert (const char *MML) {
    // Some default values
    uint8_t _adsr[4] = {0, 32, 32, 0};
    int len = 4, n;
    int8_t oct = 4;
    char c;
    bool nxt;

    mi_ind = 0;

    if (MML != NULL && IC != NULL) {
      if (*MML != 0) {
        // Analysis loop
        do {
          switch (toupper ((int) *MML)) {
            case '&': // slur
              MML++;
              if (!store_MI (0, 1)) return false;
              break;
            case '[': // repetition head
              n = 2;  // 2=infinite
              if ((c = *++MML) != 0) if (('2' <= c) && (c <= '9')) {
                n = c - '0' + 1;
                MML++;
              }
              if (!store_MI (0, n)) return false;
              break;
            case ']': // repetition tail
              MML++;
              if (!store_MI (0, 11)) return false;
              break;
            case 'V': // velocity
              n = 63;
              if ((c = *++MML) != 0) if (('0' <= c) && (c <= '9')) {
                n = c - '0';
                if ((c = *++MML) != 0) if (('0' <= c) && (c <= '9')) {
                  n = n * 10 + (c - '0');
                  if ((c = *++MML) != 0) if (('0' <= c) && (c <= '9')) {
                    n = n * 10 + (c - '0');
                    MML++;
                  }
                }
                n = MAX (MIN (n, 127), 0);
                store_MI (0, 12);
                if (!store_MI_b (n)) return false;
              }
              break;
            case '@': // wavetable changes
              n = 0;
              if ((c = *++MML) != 0) if (('0' <= c) && (c <= ('0' + _MAX_TONE - 1))) {
                n = c - '0';
                MML++;
                store_MI (0, 13);
                if (!store_MI (n, 0xf)) return false;
              }
              break;
            case 'K': // detune
              n = 8;  // 8:center
              if ((c = *++MML) != 0) if (('0' <= c) && (c <= '9')) {
                n = c - '0';
                if ((c = *++MML) != 0) if (('0' <= c) && (c <= '9')) {
                  n = n * 10 + (c - '0');
                  MML++;
                }
                store_MI (0, 14);
                if (!store_MI (n, 0xf)) return false;
              }
              break;
            case '$': // envelope
              nxt = true;
              for (int i = 0; i < 4 && nxt;i++) {
                n =_adsr[i];
                nxt = false;
                if ((c = *++MML) != 0) if ((('0' <= c) && (c <= '9')) || (c == ',')) {
                  if (c != ',') {
                    n = c - '0';
                    if ((c = *++MML) != 0) if ((('0' <= c) && (c <= '9')) || (c == ',')) {
                      if (c != ',') {
                        n = n * 10 + (c - '0');
                        if ((c = *++MML) != 0) if ((('0' <= c) && (c <= '9')) || (c == ',')) {
                          if (c != ',') {
                            n = n * 10 + (c - '0');
                            if ((c = *++MML) != 0) nxt = (c == ',');
                          } else nxt = true;
                        }
                      } else nxt = true;
                    }
                  } else nxt = true;
                }
                _adsr[i] = n;
              }
              store_MI (0, 15);
              for (int i = 0; i < 4; i++) store_MI_b (_adsr[i]);
              break;
            case 'L': // base length
              if ((c = *++MML) != 0) if (('0' <= c) && (c <= '9')) {
                n = c - '0';
                if ((c = *++MML) != 0) if (('0' <= c) && (c <= '9')) {
                  n = n * 10 + (c - '0');
                  MML++;
                }
                if (len2ind (n) != -1) len = n;
              }
              break;
            case 'R': // rest
              n = len;
              if ((c = *++MML) != 0) if ((('0' <= c) && (c <= '9')) || (c == '.')) {
                if (c == '.') n = n * 10 + 5;
                else n = c - '0';
                if ((c = *++MML) != 0) if ((('0' <= c) && (c <= '9')) || (c == '.')) {
                  if (c == '.') n = n * 10 + 5;
                  else n = n * 10 + (c - '0');
                  if ((c = *++MML) != 0) if (c == '.') {
                    n = n * 10 + 5;
                    MML++;
                  }
                }
              }
              if (!store_MI (1, len2ind ((len2ind (n) != -1) ? n : len))) return false;
              break;
            case 'C': // C
              if (!analyze_scale (&MML, oct, 2, len)) return false;
              break;
            case 'D': // D
              if (!analyze_scale (&MML, oct, 4, len)) return false;
              break;
            case 'E': // E
              if (!analyze_scale (&MML, oct, 6, len)) return false;
              break;
            case 'F': // F
              if (!analyze_scale (&MML, oct, 7, len)) return false;
              break;
            case 'G': // G
              if (!analyze_scale (&MML, oct, 9, len)) return false;
              break;
            case 'A': // A
              if (!analyze_scale (&MML, oct, 11, len)) return false;
              break;
            case 'B': // B
              if (!analyze_scale (&MML, oct, 13, len)) return false;
              break;
            case 'T': // tempo
              if ((c = *++MML) != 0) if (('0' <= c) && (c <= '9')) {
                n = c - '0';
                MML++;
                if (!store_MI (14, n)) return false;
              }
              break;
            case 'O': // octave
              if ((c = *++MML) != 0) if (('0' <= c) && (c <= '9')) {
                oct = c - '0';
                MML++;
                if (!store_MI (15, oct)) return false;
              }
              break;
            case '<': // octave up
              oct++;
              if (*++MML != 0) if (*MML == '<') {
                oct++;
                if (*++MML != 0) if (*MML == '<') {
                  oct++;
                  MML++;
                }
              }
              oct = MAX (MIN (oct, 9), 0);
              if (!store_MI (15, oct)) return false;
              break;
            case '>': // octave down
              oct--;
              if (*++MML != 0) if (*MML == '>') {
                oct--; 
                if (*++MML != 0) if (*MML == '>') {
                  oct--;
                  MML++;
                }
              }
              oct = MAX (MIN (oct, 9), 0);
              if (!store_MI (15, oct)) return false;
              break;
            default:
              MML++;
              break;
          } // switch
        } while (*MML != 0);
      } else IC[0].byte = 0;
    } else return false;
    return true;
  }

  //! play
  void CTinyMusicSq::StartMusic (void) {
    vTaskResume (TaskHandle);
  }

  //! stop
  void CTinyMusicSq::StopMusic (void) {
    if (stat != stStopping) stat = stStopExec;
    while (stat != stStopping) vTaskDelay (0);
  }

  void CTinyMusicSq::enable_PCM (void) {
    enable_pcm = true;
  }
  void CTinyMusicSq::disable_PCM (void) {
    enable_pcm = false;
  }

  void CTinyMusicSq::begin (CPCM *pcm) {
    ppcm = pcm;
    ppcm->set_custom_cb(dac_note_cb);
  }
  void CTinyMusicSq::end (void) {
    if (ppcm != NULL) {
      ppcm->set_custom_cb(NULL);
      ppcm = NULL;
    }
  }

  CTinyMusicSq::CTinyMusicSq (int sz) : stat (stStopping), enable_pcm (false) {
    ChannelNo = ChCount++;
    size = sz;
    IC = (TIntermediateCode *)new uint8_t[size];
    xTaskCreate ([](void *pmytask) {static_cast<CTinyMusicSq *>(pmytask)->playtask();}, NULL, 100, this, 1, &TaskHandle);
  }
  CTinyMusicSq::~CTinyMusicSq () {
    if (IC != NULL) delete IC;
  }

int CTinyMusicSq::ChCount = 0;
