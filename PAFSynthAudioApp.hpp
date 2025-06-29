#ifndef __PAF_SYNTH_AUDIO_APP_HPP__
#define __PAF_SYNTH_AUDIO_APP_HPP__

#include "src/memllib/audio/AudioAppBase.hpp" // Added missing include
#include "src/memllib/synth/maximilian.h" // Added missing include for maxiSettings, maxiOsc, maxiTrigger, maxiDelayline, maxiEnvGen, maxiLine

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory> // Added for std::shared_ptr

#include "src/memllib/synth/maxiPAF.hpp"
#include "src/memllib/interface/InterfaceBase.hpp" // Added missing include


class PAFSynthAudioApp : public AudioAppBase
{
public:
    static constexpr size_t kN_Params = 22;
    // static constexpr size_t nFREQs = 17;
    // static constexpr float frequencies[nFREQs] = {100, 200, 400,800, 400, 800, 100,1600,100,400,100,50,1600,200,100,800,400};
    static constexpr size_t nFREQs = 5;
    static constexpr float frequencies[nFREQs] = {100, 200, 400,800,1600};


    PAFSynthAudioApp();

    bool euclidean(float phase, const size_t n, const size_t k, const size_t offset, const float pulseWidth);

    stereosample_t __force_inline Process(const stereosample_t x) override;

    void Setup(float sample_rate, std::shared_ptr<InterfaceBase> interface) override;

    void ProcessParams(const std::vector<float>& params) override;

    queue_t qMIDINoteOn, qMIDINoteOff;

    void loop() override; 
protected:

    maxiPAFOperator paf0;
    maxiPAFOperator paf1;
    maxiPAFOperator paf2;

    maxiDelayline<5000> dl1;
    maxiDelayline<15100> dl2;

    maxiOsc pulse;
    maxiEnvGen env;


    float frame=0;

    float paf0_freq = 100;
    float paf1_freq = 100;
    float paf2_freq = 50;

    float paf0_cf = 200;
    float paf1_cf = 250;
    float paf2_cf = 250;

    float paf0_bw = 100;
    float paf1_bw = 5000;
    float paf2_bw = 5000;

    float paf0_vib = 0;
    float paf1_vib = 1;
    float paf2_vib = 1;

    float paf0_vfr = 2;
    float paf1_vfr = 2;
    float paf2_vfr = 2;

    float paf0_shift = 0;
    float paf1_shift = 0;
    float paf2_shift = 0;

    float dl1mix = 0.0f;
    float dl2mix = 0.0f;

    size_t counter=0;
    size_t freqIndex = 0;
    size_t freqOffset = 0;
    float arpFreq=50;

    maxiLine line;
    float envamp=0.f;

    float detune = 1.0;

    maxiOsc phasorOsc;
    maxiTrigger zxdetect;

    size_t euclidN=4;

    float baseFreq = 50.0f; // Base frequency for the synth
    bool newNote=false;
    float noteVel = 0.f;
    bool firstParamsReceived = false;

    float envdec=0.2f/9000.f; // Decay rate for the envelope

};

#endif  // __PAF_SYNTH_AUDIO_APP_HPP__
