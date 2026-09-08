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
                // Filter out names with non-ASCII characters to prevent tofu boxes
                bool is_ascii = true;
                for (int j = 0; name[j]; j++) {
                    if ((unsigned char)name[j] > 127) {
                        is_ascii = false;
                        break;
                    }
                }
                if (!is_ascii) continue;
                
                std::string lower_name = name;
                for (char& c : lower_name) c = tolower(c);
                if (lower_name.find("mono") == std::string::npos &&
                    lower_name.find("code") == std::string::npos &&
                    lower_name.find("console") == std::string::npos &&
                    lower_name.find("consolas") == std::string::npos &&
                    lower_name.find("courier") == std::string::npos &&
                    lower_name.find("term") == std::string::npos &&
                    lower_name.find("braille") == std::string::npos &&
                    lower_name.find("fixed") == std::string::npos &&
                    lower_name.find("hack") == std::string::npos &&
                    lower_name.find("menlo") == std::string::npos &&
                    lower_name.find("monaco") == std::string::npos &&
                    lower_name.find("inconsolata") == std::string::npos &&
                    lower_name.find("typewriter") == std::string::npos) {
                    continue;
                }

                std::string safe_name = name;
                size_t pos = 0;
                while ((pos = safe_name.find('@', pos)) != std::string::npos) {
                    safe_name.replace(pos, 1, "@@");
                    pos += 2;
                }
                font_browser_->add(safe_name.c_str(), (void*)(uintptr_t)i);
                if (current_font_name && strcmp(name, current_font_name) == 0) {
                    selected_idx = font_browser_->size();
                }
            }
        }
        font_browser_->select(selected_idx);
        if (selected_idx > 0) {
            Fl_Font f = (Fl_Font)(uintptr_t)font_browser_->data(selected_idx);
            preview_->labelfont(f);
        }
        
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
                    std::string out_name = txt;
                    size_t pos = 0;
                    while ((pos = out_name.find("@@", pos)) != std::string::npos) {
                        out_name.replace(pos, 2, "@");
                        pos += 1;
                    }
                    self->selected_font_ = out_name;
                    Fl_Font f = (Fl_Font)(uintptr_t)self->font_browser_->data(sel);
                    self->preview_->labelfont(f);
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
