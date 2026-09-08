#include "hazel/hazel.h"
#include "skred/api.h"
#include <iostream>
#include <cstring>
#include <FL/Fl.H>

int my_load_cb(hazel_app_t* app, const char* filepath, void* user_data) {
    size_t f_len = strlen(filepath);
    if (f_len < 3 || strcmp(filepath + f_len - 3, ".sk") != 0) return 0; // Let hazel handle it
    
    FILE* f = fopen(filepath, "r");
    if (!f) return 1;
    
    hazel_clear(app);
    
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#') {
            char* text = line + 1;
            if (text[0] == ' ') text++;
            hazel_append_block(app, 'D', text);
        } else {
            hazel_append_block(app, 'A', line);
        }
    }
    fclose(f);
    
    hazel_set_filepath(app, filepath);
    hazel_set_dirty(app, 0);
    return 1; // Handled
}

int my_save_cb(hazel_app_t* app, const char* filepath, void* user_data) {
    size_t f_len = strlen(filepath);
    if (f_len < 3 || strcmp(filepath + f_len - 3, ".sk") != 0) return 0; // Let hazel handle it
    
    FILE* f = fopen(filepath, "w");
    if (!f) return 1;
    
    const char* text = hazel_get_text(app);
    const char* styles = hazel_get_styles(app);
    
    if (text && styles) {
        int len = strlen(text);
        char current_style = 0;
        int block_start = 0;
        
        auto flush_block = [&](int end_pos) {
            if (block_start >= end_pos) return;
            if (current_style == 'D') {
                bool new_line = true;
                for (int i = block_start; i < end_pos; i++) {
                    if (new_line) { fprintf(f, "# "); new_line = false; }
                    fprintf(f, "%c", text[i]);
                    if (text[i] == '\n') new_line = true;
                }
                if (!new_line) fprintf(f, "\n");
            } else if (current_style == 'A') {
                for (int i = block_start; i < end_pos; i++) {
                    fprintf(f, "%c", text[i]);
                }
                if (end_pos == block_start || text[end_pos-1] != '\n') fprintf(f, "\n");
            }
        };
        
        for (int i = 0; i < len; i++) {
            char s = styles[i];
            if (s == 'C') s = 'B';
            if (s != current_style) {
                flush_block(i);
                current_style = s;
                block_start = i;
            }
        }
        flush_block(len);
    }
    
    fclose(f);
    hazel_set_filepath(app, filepath);
    hazel_set_dirty(app, 0);
    return 1; // Handled
}

void my_eval_engine(const char* input, hazel_ctx_t* ctx, void* user_data) {
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
    config.on_open = my_load_cb;
    config.on_save = my_save_cb;
    hazel_set_config(app, &config);
    
    if (argc > 1) {
        hazel_load_file(app, argv[1]);
    }
    
    hazel_run(app);
    
    skred_control_dispatch_stop();
    skred_stop();
    
    return 0;
}
