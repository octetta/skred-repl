#include "hazel/hazel.h"
#include "skred/api.h" // Assuming this is the header name from the dist
#include <iostream>

void my_eval_engine(const char* input, hazel_ctx_t* ctx, void* user_data) {
    // TODO: Wire up the skred evaluation engine here
    // We can parse ANSI colors here as discussed!
    hazel_append_output(ctx, "skred integration coming soon!\n", 0);
    hazel_finish_eval(ctx);
}

int main() {
    // Optional: Initialize skred midi or engine
    // skred_midi_init();
    
    hazel_app_t* app = hazel_create("Skred REPL", my_eval_engine, nullptr);
    
    // Optional: customize colors
    // hazel_config_t config = { ... };
    // hazel_set_config(app, &config);
    
    hazel_run(app);
    return 0;
}
