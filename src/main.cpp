#include "hazel/hazel.h"
#include "skred/api.h"
#include <iostream>
#include <cstring>
#include <FL/Fl.H>



void my_eval_engine(const char* input, hazel_ctx_t* ctx, void* user_data) {
    // Copy the input into a mutable buffer for skred_command
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s", input);
    
    // Execute the block!
    int r = skred_command(buf);
    
    // Check if the engine had anything to say
    char* log = skred_log();
    if (log && strlen(log) > 0) {
        hazel_append_output(ctx, log, 0);
    }
    
    // Optional: append return code if > 0 (as seen in mini-skred.c)
    if (r > 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "r = %d\n", r);
        hazel_append_output(ctx, msg, 0);
    } else if (r < 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "error: %d\n", r);
        hazel_append_output(ctx, msg, 1);
    }
    
    hazel_finish_eval(ctx);
}

int main() {
    // 1. Initialize the skred engine
    // (64 voices, 128 frames, UDP port 60440 - matching mini-skred defaults)
    if (skred_start(128, 64, 60440) != 0) {
        std::cerr << "Failed to start skred engine!" << std::endl;
        return 1;
    }
    
    // Enable logging so we can capture output
    skred_logger(1);
    
    // 2. Initialize the Hazel GUI
    Fl::set_font(FL_COURIER, "DejaVu Sans Mono"); // Try to explicitly grab a font with known Braille coverage
    hazel_app_t* app = hazel_create("Skred", my_eval_engine, nullptr);
    
    // 3. Set standard font size (letting Hazel handle the default crisp light theme)
    hazel_config_t config;
    config.font = FL_COURIER;
    config.font_size = 15;
    config.text_fg = FL_BLACK;
    config.input_bg = FL_WHITE;
    config.output_bg = fl_rgb_color(245, 245, 250);
    config.error_bg = fl_rgb_color(255, 235, 235);
    config.markdown_bg = fl_rgb_color(245, 255, 245);
    hazel_set_config(app, &config);
    
    // 4. Run the GUI loop
    hazel_run(app);
    
    // 5. Safely teardown
    skred_control_dispatch_stop();
    skred_stop();
    
    return 0;
}
