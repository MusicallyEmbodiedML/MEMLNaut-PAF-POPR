#include "PAFSynthAudioApp.hpp"
#include "src/memllib/synth/maximilian.h" // Required for maxiSettings etc.

// #define ARPEGGIATOR

PAFSynthAudioApp::PAFSynthAudioApp() : AudioAppBase() {}

bool PAFSynthAudioApp::euclidean(float phase, const size_t n, const size_t k, const size_t offset, const float pulseWidth)
{
    // Euclidean function
    const float fi = phase * n;
    int i = static_cast<int>(fi);
    const float rem = fi - i;
    if (i == n)
    {
        i--;
    }
    const int idx = ((i + n - offset) * k) % n;
    return (idx < k && rem < pulseWidth) ? 1 : 0;
}

stereosample_t PAFSynthAudioApp::Process(const stereosample_t x)
{
    float x1[1];

    // const float trig = pulse.square(1);

    paf0.play(x1, 1, baseFreq, baseFreq + (paf0_cf * baseFreq), paf0_bw * baseFreq, paf0_vib, paf0_vfr, paf0_shift, 0);
    float y = x1[0];

    const float freq1 = baseFreq * detune;

    paf1.play(x1, 1, freq1, freq1 + (paf1_cf * freq1), paf1_bw * freq1, paf1_vib, paf1_vfr, paf1_shift, 1);
    y += x1[0];

    const float freq2 = freq1 * detune;

    paf2.play(x1, 1, freq2, freq2 + (paf2_cf * freq2), paf2_bw * freq2, paf2_vib, paf2_vfr, paf2_shift, 1);
    y += x1[0];

#ifdef ARPEGGIATOR
    const float ph = phasorOsc.phasor(1);
    const bool euclidNewNote = euclidean(ph, 12, euclidN, 0, 0.1f);
    if(zxdetect.onZX(euclidNewNote)) {
        envamp=0.5f;
        freqIndex++;
        if(freqIndex >= nFREQs) {
            freqIndex = 0;
        }
        baseFreq = frequencies[freqIndex];
   }else{
        // constexpr float envdec = 0.2f/9000.f;
        envamp -= envdec;
        if (envamp < 0.f) {
            envamp = 0.f;
        }
    }
#else
    if(newNote) {
        newNote = false;
        envamp=0.8f;
   }else{
        // constexpr float envdec = 0.2f/9000.f;
        envamp -= envdec;
        if (envamp < 0.f) {
            envamp = 0.f;
        }
    }
#endif

    // PERIODIC_DEBUG(3000, Serial.println(y);)
    y = y * envamp* envamp;
    // counter++;
    // if(counter>=9000) {
    //     counter=0;
    //     freqIndex++;
    //     if(freqIndex >= 4) {
    //         freqIndex = 0;
    //     }
    //     arpFreq = frequencies[freqIndex];
    // }

    // PERIODIC_DEBUG(10000, {
    //     Serial.println(envamp);
    // })
#ifndef ARPEGGIATOR
    y *= noteVel;
#endif

    float d1 = (dl1.play(y, 3500, 0.8f) * dl1mix);
    // float d2 = (dl2.play(y, 15000, 0.8f) * dl2mix);
    y = y + d1;// + d2;
    stereosample_t ret { y, y };
    frame++;
    return ret;
}

void PAFSynthAudioApp::Setup(float sample_rate, std::shared_ptr<InterfaceBase> interface)
{
    AudioAppBase::Setup(sample_rate, interface);
    maxiSettings::sampleRate = sample_rate;


    paf0.init();
    paf0.setsr(maxiSettings::getSampleRate(), 1);
    // paf0.freq(100, 0);
    // // paf0.amp(1,0);
    // paf0.bw(200,0);
    // paf0.cf(210,0);
    // paf0.vfr(5,0);
    // paf0.vib(0.1,0);
    // paf0.shift(10,0);

    paf1.init();
    paf1.setsr(maxiSettings::getSampleRate(), 1);
    // paf1.freq(150, 0);
    // // paf1.amp(1,0);
    // paf1.bw(200,0);
    // paf1.cf(210,0);
    // paf1.vfr(5,0);
    // paf1.vib(0.1,0);
    // paf1.shift(10,0);

    paf2.init();
    paf2.setsr(maxiSettings::getSampleRate(), 1);
    // paf2.freq(190, 0);
    // // paf2.amp(1,0);
    // paf2.bw(500,0);
    // paf2.cf(210,0);
    // paf2.vfr(5,0);
    // paf2.vib(0.1,0);
    // paf2.shift(6,0);

    env.setupAR(10,100);
    arpFreq = frequencies[0];
    // line.prepare(1.f,0.f,100.f,false);
    // line.triggerEnable(true);
    envamp=1.f;

    queue_init(&qMIDINoteOn, sizeof(uint8_t)*2, 1);

}

float mtof(uint8_t note) {
    // Convert MIDI note to frequency
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

void PAFSynthAudioApp::loop(){
    AudioAppBase::loop();
    uint8_t midimsg[2];

    if (firstParamsReceived && queue_try_remove(&qMIDINoteOn, &midimsg)) {
        // Serial.printf("PAFSynthAudioApp::ProcessParams - Received MIDI Note On: %d, Velocity: %d\n", midimsg[0], midimsg[1]);
        baseFreq = mtof(midimsg[0]);
        noteVel = midimsg[1] / 127.0f; // Normalize velocity to [0, 1]
        noteVel = noteVel * noteVel; // Square the velocity for more pronounced effect
        newNote = true;
    }
}


void PAFSynthAudioApp::ProcessParams(const std::vector<float>& params)
{
    firstParamsReceived = true;
    // // Map parameters to the synth
    // synth_.mapParameters(params);
    // //Serial.print("Params processed.");
    // paf0_freq = 50.f + (params[0] * params[0] * 1000.f);
    // paf1_freq = 50.f + (params[1] * params[1] * 1000.f);

    // paf0_cf = arpFreq + (params[2] * params[2] * arpFreq * 1.f);
    // paf1_cf = arpFreq + (params[3] * params[3] * arpFreq * 1.f);
    // paf2_cf = arpFreq + (params[4] * params[4] * arpFreq * 1.f);
    paf0_cf = (params[2] * params[2]  * 2.f);
    paf1_cf = (params[3] * params[3]  * 2.f);
    paf2_cf = (params[4] * params[4]  * 2.f);

    // paf0_bw = 5.f + (params[5] * arpFreq * 0.5f);
    // paf1_bw = 5.f + (params[6] * arpFreq * 0.5f);
    // paf2_bw = 5.f + (params[7] * arpFreq * 0.5f);
    paf0_bw = 0.1f + (params[5] * 2.f);
    paf1_bw = 0.1f + (params[6] * 2.f);
    paf2_bw = 0.1f + (params[7] * 2.f);

    paf0_vib = (params[8] * params[8] * 0.99f);
    paf1_vib = (params[9] * params[9] * 0.99f);
    paf2_vib = (params[10] * params[10] * 0.99f);

    paf0_vfr = (params[11] * params[11]* 10.f);
    paf1_vfr = (params[12] * params[12] * 10.f);
    paf2_vfr = (params[13] * params[13] * 10.f);

    paf0_shift = (params[14] * 1000.f);
    paf1_shift = (params[15] * 1000.f);
    paf2_shift = (params[16] * 1000.f);

    dl1mix = params[17] * params[17] * 0.4f;
    // dl2mix = params[18] * params[18] * 0.4f;
    detune = 1.0f + (params[18] * 0.1);

    euclidN = static_cast<size_t>(2 + (params[19] * 5));
    
    envdec=((params[20] * 15.f) + 0.1f)/9000.f; // Decay rate for the envelope


    // Serial.printf("%f %f %f %f %f\n", paf0_cf,  paf0_bw, paf0_vib, paf0_vfr, paf0_shift);
}
