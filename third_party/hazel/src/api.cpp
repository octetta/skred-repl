#include "hazel/hazel.h"
#include "HazelApp.h"
#include <FL/Fl.H>
#include <string.h>

extern "C" {

void hazel_append_output(hazel_ctx_t* ctx, const char* text, int is_error) {
    printf("hazel_append_output called! ctx=%p ctx->app=%p text=%s\n", ctx, ctx ? ctx->app : NULL, text);
    if (!ctx || !ctx->app || !text) return;
    
    // We must lock the GUI if called from an async thread
    Fl::lock();
    
    ctx->app->appendOutput(ctx->insert_pos, text, is_error);
    ctx->insert_pos += strlen(text);
    
    Fl::unlock();
    Fl::awake();
}

void hazel_finish_eval(hazel_ctx_t* ctx) {
    if (ctx) {
        HazelApp* app = ctx->app;
        Fl::lock();
        app->finishEvaluation(ctx);
        Fl::unlock();
        Fl::awake();
        delete ctx;
    }
}

hazel_app_t* hazel_create(const char* title, hazel_eval_cb_t eval_cb, void* user_data) {
    HazelApp* app = new HazelApp(title, eval_cb, user_data);
    return (hazel_app_t*)app;
}

int hazel_run(hazel_app_t* app) {
    if (!app) return -1;
    return ((HazelApp*)app)->run();
}

void hazel_destroy(hazel_app_t* app) {
    if (app) {
        delete (HazelApp*)app;
    }
}

void hazel_load_file(hazel_app_t* app, const char* filepath) {
    if (app) {
        ((HazelApp*)app)->loadFile(filepath);
    }
}

}

const char* hazel_version() {
    return HAZEL_VERSION;
}

void hazel_set_config(hazel_app_t* app, const hazel_config_t* config) {
    if (app && config) {
        ((HazelApp*)app)->setConfig(config);
    }
}
