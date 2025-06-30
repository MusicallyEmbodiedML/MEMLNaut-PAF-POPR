// #include "src/memllib/interface/InterfaceBase.hpp"
#include "src/memllib/interface/MIDIInOut.hpp"
#include "src/memllib/hardware/memlnaut/display.hpp"
#include "src/memllib/audio/AudioAppBase.hpp"
#include "src/memllib/audio/AudioDriver.hpp"
#include "src/memllib/hardware/memlnaut/MEMLNaut.hpp"
#include <memory>
#include "hardware/structs/bus_ctrl.h"
#include "PAFSynthAudioApp.hpp"
#include "src/memllib/examples/IMLInterface.hpp"


#define APP_SRAM __not_in_flash("app")

const char FIRMWARE_NAME[] = "-- PAF synth POPR --";

bool core1_disable_systick = true;
bool core1_separate_stack = true;

uint32_t get_rosc_entropy_seed(int bits) {
    uint32_t seed = 0;
    for (int i = 0; i < bits; ++i) {
        // Wait for a bit of time to allow jitter to accumulate
        busy_wait_us_32(5);
        // Pull LSB from ROSC rand output
        seed <<= 1;
        seed |= (rosc_hw->randombit & 1);
    }
    return seed;
}


// Global objects
std::shared_ptr<IMLInterface> APP_SRAM interface;
std::shared_ptr<display> APP_SRAM scr;

std::shared_ptr<MIDIInOut> APP_SRAM midi_interf;


std::shared_ptr<PAFSynthAudioApp> __scratch_y("audio") audio_app;

// Inter-core communication
volatile bool APP_SRAM core_0_ready = false;
volatile bool APP_SRAM core_1_ready = false;
volatile bool APP_SRAM serial_ready = false;
volatile bool APP_SRAM interface_ready = false;



// We're only bound to the joystick inputs (x, y, rotate)
constexpr size_t kN_InputParams = 3;

// Add these macros near other globals
#define MEMORY_BARRIER() __sync_synchronize()
#define WRITE_VOLATILE(var, val) do { MEMORY_BARRIER(); (var) = (val); MEMORY_BARRIER(); } while (0)
#define READ_VOLATILE(var) ({ MEMORY_BARRIER(); typeof(var) __temp = (var); MEMORY_BARRIER(); __temp; })


struct repeating_timer APP_SRAM timerDisplay;
inline bool __not_in_flash_func(displayUpdate)(__unused struct repeating_timer *t) {
    scr->update();
    return true;
}

void setup()
{
    scr = std::make_shared<display>();
    scr->setup();
    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_W_BITS |
        BUSCTRL_BUS_PRIORITY_DMA_R_BITS | BUSCTRL_BUS_PRIORITY_PROC1_BITS;

    uint32_t seed = get_rosc_entropy_seed(32);
    srand(seed);

    Serial.begin(115200);
    // while (!Serial) {}
    Serial.println("Serial initialised.");
    WRITE_VOLATILE(serial_ready, true);

    // Setup board
    MEMLNaut::Initialize();
    pinMode(33, OUTPUT);

    auto temp_interface = std::make_shared<IMLInterface>();
    temp_interface->setup(kN_InputParams, PAFSynthAudioApp::kN_Params, scr);
    MEMORY_BARRIER();
    interface = temp_interface;
    MEMORY_BARRIER();

    // Setup interface with memory barrier protection
    WRITE_VOLATILE(interface_ready, true);
    // Bind interface after ensuring it's fully initialized
    interface->bindInterface();
    Serial.println("Bound RL interface to MEMLNaut.");

    midi_interf = std::make_shared<MIDIInOut>();
    midi_interf->Setup(0);
    midi_interf->SetMIDISendChannel(1);
    Serial.println("MIDI setup complete.");
    if (midi_interf) {
        midi_interf->SetNoteCallback([interface] (bool noteon, uint8_t note_number, uint8_t vel_value) {
        if (noteon) {
            uint8_t midimsg[2] = {note_number, vel_value };
            queue_try_add(&audio_app->qMIDINoteOn, &midimsg);
        }
            Serial.printf("MIDI Note %d: %d\n", note_number, vel_value);
        });
        Serial.println("MIDI note callback set.");
    }



    WRITE_VOLATILE(core_0_ready, true);
    while (!READ_VOLATILE(core_1_ready)) {
        MEMORY_BARRIER();
        delay(1);
    }

    scr->post(FIRMWARE_NAME);
    add_repeating_timer_ms(-39, displayUpdate, NULL, &timerDisplay);

    Serial.println("Finished initialising core 0.");
}

void loop()
{


    MEMLNaut::Instance()->loop();
    static int AUDIO_MEM blip_counter = 0;
    if (blip_counter++ > 100) {
        blip_counter = 0;
        Serial.println(".");
        // Blink LED
        digitalWrite(33, HIGH);
    } else {
        // Un-blink LED
        digitalWrite(33, LOW);
    }
    midi_interf->Poll();
    delay(10); // Add a small delay to avoid flooding the serial output
}

void setup1()
{
    while (!READ_VOLATILE(serial_ready)) {
        MEMORY_BARRIER();
        delay(1);
    }

    while (!READ_VOLATILE(interface_ready)) {
        MEMORY_BARRIER();
        delay(1);
    }


    // Create audio app with memory barrier protection
    {
        auto temp_audio_app = std::make_shared<PAFSynthAudioApp>();

        temp_audio_app->Setup(AudioDriver::GetSampleRate(), interface);
        MEMORY_BARRIER();
        audio_app = temp_audio_app;
        MEMORY_BARRIER();
    }

    // Start audio driver
    AudioDriver::Setup();

    WRITE_VOLATILE(core_1_ready, true);
    while (!READ_VOLATILE(core_0_ready)) {
        MEMORY_BARRIER();
        delay(1);
    }

    Serial.println("Finished initialising core 1.");
}

void loop1()
{
    // Audio app parameter processing loop
    audio_app->loop();
    delay(1);
}

