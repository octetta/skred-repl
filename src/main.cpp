#include <cstdio>
#include <iostream>
#include <cstring>
#include "hazel/hazel.h"
#include "skred/api.h"
#include <FL/Fl.H>

int my_load_cb(hazel_app_t* app, const char* filepath, void* user_data) { 
    size_t f_len = strlen(filepath);
    if (f_len < 3 || strcmp(filepath + f_len - 3, ".sk") != 0) return 0;
    
    FILE* f = fopen(filepath, "r");
    if (!f) return 1;
    
    hazel_clear(app);
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* buf = (char*)malloc(size + 1);
    if (buf) {
        fread(buf, 1, size, f);
        buf[size] = '\0';
        hazel_append_block(app, 'A', buf); // Will be re-parsed by style_update_cb if we trigger it
        free(buf);
    }
    fclose(f);
    
    hazel_set_filepath(app, filepath);
    hazel_set_dirty(app, 0);
    return 1;
}

int my_save_cb(hazel_app_t* app, const char* filepath, void* user_data) {
    size_t f_len = strlen(filepath);
    if (f_len < 3 || strcmp(filepath + f_len - 3, ".sk") != 0) return 0;
    
    FILE* f = fopen(filepath, "w");
    if (!f) return 1;
    
    const char* text = hazel_get_text(app);
    const char* styles = hazel_get_styles(app);
    
    if (text && styles) {
        int len = strlen(text);
        for (int i = 0; i < len; i++) {
            char s = styles[i];
            if (s == 'A' || s == 'D') {
                fprintf(f, "%c", text[i]);
            }
        }
    }
    
    fclose(f);
    hazel_set_filepath(app, filepath);
    hazel_set_dirty(app, 0);
    return 1;
}

void my_eval_engine(const char* input, hazel_ctx_t* ctx, void* user_data) {
    
    // ========================================================================
    // INTERCEPT HOOK:
    // This is the ideal place to intercept the entered string `input` BEFORE 
    // it gets sent to the skred engine.
    //
    // For example, if you wanted a custom command to restart the engine:
    // 
    // if (strncmp(input, "!restart", 8) == 0) {
    //     skred_stop();
    //     skred_start(128, 64, 60440);
    //
    //     // To output to the Notebook (or Terminal if the command came from there):
    //     // arg 1: output string
    //     // arg 2: 0 for normal output, 1 for error (red text)
    //     hazel_append_output(ctx, "Skred engine restarted successfully.\n", 0);
    //
    //     // ALWAYS call finish_eval so the UI knows the execution completed
    //     hazel_finish_eval(ctx);
    //     return; 
    // }
    // ========================================================================

    char buf[1024];
    snprintf(buf, sizeof(buf), "%s", input);
    
    int r = skred_command(buf);
    
    char* log = skred_log();
    if (log && strlen(log) > 0) {
        hazel_append_output(ctx, log, 0);
    }
    
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

int main(int argc, char** argv) {
    if (skred_start(128, 64, 60440) != 0) {
        std::cerr << "Failed to start skred engine!" << std::endl;
        return 1;
    }
    
    skred_logger(1);
    
    Fl::set_font(FL_COURIER, "DejaVu Sans Mono");
    hazel_app_t* app = hazel_create("Skred", my_eval_engine, nullptr);
    
    hazel_config_t config;
    memset(&config, 0, sizeof(config));
    config.font = FL_COURIER;
    config.font_size = 15;
    config.text_fg = FL_BLACK;
    config.input_bg = FL_WHITE;
    config.output_bg = fl_rgb_color(245, 245, 250);
    config.error_bg = fl_rgb_color(255, 235, 235);
    config.markdown_bg = fl_rgb_color(245, 255, 245);
    config.error_fg = FL_DARK_RED;
    config.markdown_fg = FL_DARK_GREEN;
    config.cursor_fg = FL_WHITE;
    config.cursor_bg = FL_BLACK;
    config.select_bg = fl_rgb_color(180, 200, 255);
    config.parser_mode = 1;
    config.on_open = my_load_cb;
    config.on_save = my_save_cb;
    hazel_set_config(app, &config);
    hazel_load_preferences(app);
    
    if (argc > 1) {
        hazel_load_file(app, argv[1]);
        
    }
    
    hazel_run(app);
    
    skred_control_dispatch_stop();
    skred_stop();
    
    return 0;
}
