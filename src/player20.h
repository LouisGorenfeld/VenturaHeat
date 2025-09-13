#ifndef _RADPLAY_
#define _RADPLAY_

#include <stdio.h>
#include <dpmi.h>
#include <pc.h>


typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef char int8_t;
typedef unsigned char uint8_t;

        extern FILE *gLogFP;

extern int ADLIB;
//extern int gAdlibCommands;

class RADPlayer {

    // Various constants
    enum {
        kTracks             = 100,
        kChannels           = 9,
        kTrackLines         = 64,
        kRiffTracks         = 10,
        kInstruments        = 127,

        cmPortamentoUp      = 0x1,
        cmPortamentoDwn     = 0x2,
        cmToneSlide         = 0x3,
        cmToneVolSlide      = 0x5,
        cmVolSlide          = 0xA,
        cmSetVol            = 0xC,
        cmJumpToLine        = 0xD,
        cmSetSpeed          = 0xF,
        cmIgnore            = ('I' - 55),
        cmMultiplier        = ('M' - 55),
        cmRiff              = ('R' - 55),
        cmTranspose         = ('T' - 55),
        cmFeedback          = ('U' - 55),
        cmVolume            = ('V' - 55),
    };

    enum e_Source {
        SNone, SRiff, SIRiff,
    };

    enum {
        fKeyOn              = 1 << 0,
        fKeyOff             = 1 << 1,
        fKeyedOn            = 1 << 2,
    };

    struct CInstrument {
        uint8_t             Feedback[2];
        uint8_t             Panning[2];
        uint8_t             Algorithm;
        uint8_t             Detune;
        uint8_t             Volume;
        uint8_t             RiffSpeed;
        uint8_t *           Riff;
        uint8_t             Operators[4][5];
    };

    struct CEffects {
        int8_t              PortSlide;
        int8_t              VolSlide;
        uint16_t            ToneSlideFreq;
        uint8_t             ToneSlideOct;
        uint8_t             ToneSlideSpeed;
        int8_t              ToneSlideDir;
    };

    struct CChannel {
        uint8_t             LastInstrument;
        CInstrument *       Instrument;
        uint8_t             Volume;
        uint8_t             DetuneA;
        uint8_t             DetuneB;
        uint8_t             KeyFlags;
        uint16_t            CurrFreq;
        int8_t              CurrOctave;
        CEffects            FX;
        struct CRiff {
            CEffects        FX;
            uint8_t *       Track;
            uint8_t *       TrackStart;
            uint8_t         Line;
            uint8_t         Speed;
            uint8_t         SpeedCnt;
            int8_t          TransposeOctave;
            int8_t          TransposeNote;
            uint8_t         LastInstrument;
        } Riff, IRiff;
    };

    public:
                            RADPlayer() : Initialised(false) {}
        void                Init(const void *tune, void *arg);
        void                Stop();
        bool                Update();
        int                 GetHertz() const {  return Hertz;  }
        int                 GetPlayTimeInSeconds() const {  return PlayTime / Hertz;  }
        int                 GetTunePos() const {  return Order;  }
        int                 GetTuneLength() const {  return OrderListSize;  }
        int                 GetTuneLine() const {  return Line;  }
        void                SetMasterVolume(int vol) {  MasterVol = vol;  }
        int                 GetMasterVolume() const {  return MasterVol;  }
        int                 GetSpeed() const {  return Speed;  }

#if RAD_DETECT_REPEATS
        uint32_t            ComputeTotalTime();
#endif

        void                PlayNote(int channum, int8_t notenum, int8_t octave, uint16_t instnum, uint8_t cmd = 0, uint8_t param = 0, e_Source src = SNone, int op = 0);

        // For sound effects; these don't touch player data
        void                SetFrequencyDirect(uint16_t channum, uint16_t f);


    private:
        bool                UnpackNote(uint8_t *&s, uint8_t &last_instrument);
        uint8_t *           GetTrack();
        uint8_t *           SkipToLine(uint8_t *trk, uint8_t linenum, bool chan_riff = false);
        void                PlayLine();
        void                LoadInstrumentOPL3(int channum);
        void                PlayNoteOPL3(int channum, int8_t octave, int8_t note);
        void                ResetFX(CEffects *fx);
        void                TickRiff(int channum, CChannel::CRiff &riff, bool chan_riff);
        void                ContinueFX(int channum, CEffects *fx);
        void                SetVolume(int channum, uint8_t vol);
        void                GetSlideDir(int channum, CEffects *fx);
        void                LoadInstMultiplierOPL3(int channum, int op, uint8_t mult);
        void                LoadInstVolumeOPL3(int channum, int op, uint8_t vol);
        void                LoadInstFeedbackOPL3(int channum, int which, uint8_t fb);
        void                Portamento(uint16_t channum, CEffects *fx, int8_t amount, bool toneslide);
        void                Transpose(int8_t note, int8_t octave);
        inline void                SetOPL3(uint16_t reg, uint8_t data) {
                                         OPL3Regs[reg] = data;
                                 int offset = (reg > 0xFF) ? 2 : 0;

        #ifdef CLONEMODE
                                 outportb(ADLIB + offset, reg);

                                 outportb(ADLIB + offset + 1, data);

                                 // Wait
                                 for (int i=0; i < 4; i++)
                                      inportb(ADLIB + offset);
        #else


                                 outportb(ADLIB + offset, reg);
                                 for (int i=0; i < 6; i++)
                                      inportb(ADLIB + offset);

                                 outportb(ADLIB + offset + 1, data);
                                 for (int i=0; i < 6; i++)
                                      inportb(ADLIB + offset);

        #endif

//                                gAdlibCommands++;
                            }
        uint8_t             GetOPL3(uint16_t reg) const {
                                return OPL3Regs[reg];
                            }

        void                (*OPL3)(void *, uint16_t, uint8_t);
        void *              OPL3Arg;
        CInstrument         Instruments[kInstruments];
        CChannel            Channels[kChannels];
        uint32_t            PlayTime;
#if RAD_DETECT_REPEATS
        uint32_t            OrderMap[4];
        bool                Repeating;
#endif
        int16_t             Hertz;
        uint8_t *           OrderList;
        uint8_t *           Tracks[kTracks];
        uint8_t *           Riffs[kRiffTracks][kChannels];
        uint8_t *           Track;
        bool                Initialised;
        uint8_t             Speed;
        uint8_t             OrderListSize;
        uint8_t             SpeedCnt;
        uint8_t             Order;
        uint8_t             Line;
        int8_t              Entrances;
        uint8_t             MasterVol;
        int8_t              LineJump;
        uint8_t             OPL3Regs[512];

        // Values exported by UnpackNote()
        int8_t              NoteNum;
        int8_t              OctaveNum;
        uint8_t             InstNum;
        uint8_t             EffectNum;
        uint8_t             Param;
        bool                LastNote;

        static const int8_t NoteSize[];
        static const uint16_t ChanOffsets3[9], Chn2Offsets3[9];
        static const uint16_t NoteFreq[];
        static const uint16_t OpOffsets3[9][4];
        static const bool   AlgCarriers[7][4];
};


#endif
