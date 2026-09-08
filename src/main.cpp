#include "hazel/hazel.h"
#include "skred/api.h"
#include <iostream>
#include <cstring>
#include <FL/Fl.H>

void parse_and_append_ansi(hazel_ctx_t* ctx, const char* text) {
    // For now, strip ANSI colors and just append raw text.
    // We will implement the full dynamic style cache in the next iteration.
    char* clean = strdup(text);
    char* src = clean;
    char* dst = clean;
    while (*src) {
        if (*src == '\x1b' && *(src + 1) == '[') {
            while (*src && *src != 'm') src++;
            if (*src == 'm') src++;
            continue;
        }
        *dst++ = *src++;
    }
    *dst = '\0';
    if (strlen(clean) > 0) {
        hazel_append_output(ctx, clean, 0);
    }
    free(clean);
}

void my_eval_engine(const char* input, hazel_ctx_t* ctx, void* user_data) {
    // Copy the input into a mutable buffer for skred_command
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s", input);
    
    // Execute the block!
    int r = skred_command(buf);
    
    // Check if the engine had anything to say
    char* log = skred_log();
    if (log && strlen(log) > 0) {
        parse_and_append_ansi(ctx, log);
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
    hazel_app_t* app = hazel_create("Skred", my_eval_engine, nullptr);
    
    // 3. Customize theme to match synth aesthetic
    hazel_config_t config;
    config.font = FL_COURIER;
    config.font_size = 15;
    config.text_fg = fl_rgb_color(220, 220, 220);
    config.input_bg = fl_rgb_color(25, 25, 30);
    config.output_bg = fl_rgb_color(15, 15, 20);
    config.error_bg = fl_rgb_color(40, 10, 10);
    config.markdown_bg = fl_rgb_color(20, 30, 25);
    hazel_set_config(app, &config);
    
    // 4. Run the GUI loop
    hazel_run(app);
    
    // 5. Safely teardown
    skred_control_dispatch_stop();
    skred_stop();
    
    return 0;
}
