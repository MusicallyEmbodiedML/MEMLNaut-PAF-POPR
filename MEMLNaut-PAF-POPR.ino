// #include "src/memllib/hardware/memlnaut/display/DisplayDriver.hpp"
// #include "src/memllib/hardware/memlnaut/display/TextView.hpp"
#include "src/memllib/hardware/memlnaut/display/MessageView.hpp"
#include "src/memllib/interface/MIDIInOut.hpp"
// #include "src/memllib/hardware/memlnaut/display.hpp"
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
// std::shared_ptr<display> APP_SRAM scr;

std::shared_ptr<MIDIInOut> APP_SRAM midi_interf;


std::shared_ptr<PAFSynthAudioApp> __scratch_y("audio") audio_app;

// std::shared_ptr<DisplayDriver> APP_SRAM disp;

std::shared_ptr<MessageView> APP_SRAM midiView;

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


// struct repeating_timer APP_SRAM timerDisplay;
// inline bool __not_in_flash_func(displayUpdate)(__unused struct repeating_timer *t) {
//     disp->Draw();
//     return true;
// }

// struct repeating_timer APP_SRAM timerTouch;
// inline bool __not_in_flash_func(touchUpdate)(__unused struct repeating_timer *t) {
//     // scr->update();
//     disp->PollTouch();
//     return true;
// }
// struct repeating_timer APP_SRAM timerMIDIIn;
// inline bool __not_in_flash_func(MIDIPoll)(__unused struct repeating_timer *t) {
//     midi_interf->Poll();
//     return true;
// }


void setup()
{
    set_sys_clock_khz(AudioDriver::GetSysClockSpeed(), true);

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
    temp_interface->setup(kN_InputParams, PAFSynthAudioApp::kN_Params);
    MEMORY_BARRIER();
    interface = temp_interface;
    MEMORY_BARRIER();


    // Setup interface with memory barrier protection
    WRITE_VOLATILE(interface_ready, true);
    // Bind interface after ensuring it's fully initialized
    interface->bindInterface();
    Serial.println("Bound RL interface to MEMLNaut.");

    // // Create test views - now using string literals
    // auto view1 = std::make_shared<TextView>("View 1", "Hello World!", TFT_RED);
    // auto view2 = std::make_shared<TextView>("View 2", "Touch Me!", TFT_GREEN);
    // auto view3 = std::make_shared<TextView>("View 3", "Last View", TFT_BLUE);
    // auto msgView = std::make_shared<MessageView>("PAF Synth");

    // disp = std::make_shared<DisplayDriver>();
    // // Add views to display
    // disp->AddView(view1);
    // disp->AddView(view2);
    // disp->AddView(view3);
    // disp->AddView(msgView);
    // disp->Setup();

    // midi_interf = std::make_shared<MIDIInOut>();
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

    std::shared_ptr<MessageView> helpView = std::make_shared<MessageView>("Help");
    helpView->post("PAF synth POPR");
    helpView->post("TB: Up: Train, Down: Inference");
    helpView->post("TA: Zoom On/Off");
    helpView->post("MB: Up: Randomise in Training or Inference");
    helpView->post("MB: Down: Clear Dataset");
    helpView->post("Y: Zoom Factor");
    helpView->post("Z: Training Iterations");
    helpView->post("Joystick: Explore");
    helpView->post("MIDI: Pitch and velocity");
    MEMLNaut::Instance()->disp->AddView(helpView);
    MEMLNaut::Instance()->addSystemInfoView();

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

    midi_interf = std::make_shared<MIDIInOut>();
    // midiView = std::make_shared<MessageView>("MIDI Monitor");
    // midiView->setMaxLines(5);
    // midiView->setLineWidth(100);
    // MEMLNaut::Instance()->disp->AddView(midiView);

    midi_interf->Setup(0);
    midi_interf->SetMIDISendChannel(1);
    if (midi_interf) {
        // midiView->post("MIDI interface ready.");
        midi_interf->SetNoteCallback([] (bool noteon, uint8_t note_number, uint8_t vel_value) {
            if (noteon) {
                uint8_t midimsg[2] = {note_number, vel_value };
                queue_try_add(&audio_app->qMIDINoteOn, &midimsg);
            }
            // midiView->post("Note " + String(note_number) + ": " + String(vel_value));
        });
        // add_repeating_timer_ms(9, MIDIPoll, NULL, &timerMIDIIn);
        // midiView->post("Listening on all channels");
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

size_t midiCounter=0;
void loop1()
{
    // Audio app parameter processing loop
    audio_app->loop();
    if(midiCounter++ == 5) {
        midiCounter=0;
        midi_interf->Poll();
    }
    delay(1);
}

