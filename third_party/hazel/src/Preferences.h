#pragma once
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include "hazel/hazel.h"
#include <string>

class PreferencesWindow : public Fl_Double_Window {
    Fl_Hold_Browser* font_browser_;
    Fl_Choice* theme_choice_;
    Fl_Box* preview_;
    Fl_Button* ok_;
    Fl_Button* cancel_;
    
    std::string selected_font_;
    int selected_theme_;
    bool applied_ = false;

public:
    PreferencesWindow(const hazel_config_t& current_cfg) : Fl_Double_Window(420, 360, "Preferences") {
        new Fl_Box(10, 10, 400, 20, "Select Font:");
        font_browser_ = new Fl_Hold_Browser(10, 30, 400, 180);
        font_browser_->has_scrollbar(Fl_Browser_::BOTH);
        font_browser_->format_char(0);
        
        new Fl_Box(10, 220, 100, 25, "Theme:");
        theme_choice_ = new Fl_Choice(110, 220, 300, 25);
        theme_choice_->add("Hazel Light");
        theme_choice_->add("Synth Dark");
        theme_choice_->value(0); // Default to light
        
        preview_ = new Fl_Box(10, 260, 400, 40, "⢀⣴⣾⣿⣿⣷⣦⡀ ⣾⣿ Braille Test");
        preview_->box(FL_DOWN_BOX);
        preview_->color(FL_WHITE);
        
        cancel_ = new Fl_Button(240, 315, 80, 30, "Cancel");
        ok_ = new Fl_Button(330, 315, 80, 30, "OK");
        
        int num_fonts = Fl::set_fonts("-*");
        const char* current_font_name = Fl::get_font_name(current_cfg.font);
        int selected_idx = 1;
        for (int i = 0; i < num_fonts; i++) {
            const char* name = Fl::get_font_name((Fl_Font)i);
            if (name) {
                font_browser_->add(name);
                if (current_font_name && strcmp(name, current_font_name) == 0) {
                    selected_idx = font_browser_->size();
                }
            }
        }
        font_browser_->select(selected_idx);
        
        if (current_cfg.input_bg == FL_WHITE) {
            theme_choice_->value(0); // Light
        } else {
            theme_choice_->value(1); // Dark
        }
        
        font_browser_->callback([](Fl_Widget*, void* v) {
            PreferencesWindow* self = (PreferencesWindow*)v;
            int sel = self->font_browser_->value();
            if (sel > 0) {
                const char* txt = self->font_browser_->text(sel);
                if (txt) {
                    self->selected_font_ = txt;
                    self->preview_->labelfont(FL_FREE_FONT);
                    Fl::set_font(FL_FREE_FONT, txt);
                    self->preview_->redraw();
                }
            }
        }, this);
        
        ok_->callback([](Fl_Widget*, void* v) {
            PreferencesWindow* self = (PreferencesWindow*)v;
            self->selected_theme_ = self->theme_choice_->value();
            self->applied_ = true;
            self->hide();
        }, this);
        
        cancel_->callback([](Fl_Widget*, void* v) {
            ((Fl_Window*)v)->hide();
        }, this);
        
        end();
    }
    
    bool run(std::string& out_font, int& out_theme) {
        applied_ = false;
        show();
        while (shown()) Fl::wait();
        if (applied_) {
            out_font = selected_font_;
            out_theme = selected_theme_;
            return true;
        }
        return false;
    }
};
