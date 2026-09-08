#include "TerminalPane.h"
#include "HazelApp.h"
#include "hazel/hazel.h"
#include <iostream>



void TerminalPane::style_update_cb(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg) {
    // No-op for now, just required for FLTK style buffers
}

TerminalPane::TerminalPane(int X, int Y, int W, int H, HazelApp* app) 
    : Fl_Text_Editor(X, Y, W, H), app_(app), history_index_(0), prompt_pos_(0) {
    
    buf_ = new Fl_Text_Buffer();
    style_buf_ = new Fl_Text_Buffer();
    
    this->buffer(buf_);
    this->box(FL_FLAT_BOX);
    this->color(FL_BLACK); // Classic terminal look
    this->textcolor(FL_WHITE);
    this->textfont(FL_COURIER);
    this->textsize(14);
    this->cursor_color(FL_WHITE);
    this->cursor_style(Fl_Text_Display::BLOCK_CURSOR);
    
    buf_->add_modify_callback(style_update_cb, this);
    
    // We could use highlight_data to style errors in red, etc.
    // For now, pure monochrome text works, or we can just use the FLTK styles.
    
    printPrompt();
}

TerminalPane::~TerminalPane() {
    delete buf_;
    delete style_buf_;
}

void TerminalPane::printPrompt() {
    const char* prompt = "skred> ";
    buf_->remove_modify_callback(style_update_cb, this);
    int p = buf_->length();
    buf_->insert(p, prompt);
    buf_->add_modify_callback(style_update_cb, this);
    
    prompt_pos_ = buf_->length();
    insert_position(prompt_pos_);
    show_insert_position();
}

void TerminalPane::appendOutput(const char* text, bool is_error) {
    if (!text || strlen(text) == 0) return;
    buf_->remove_modify_callback(style_update_cb, this);
    int p = buf_->length();
    buf_->insert(p, text);
    
    if (text[strlen(text)-1] != '\n') {
        buf_->insert(buf_->length(), "\n");
    }
    buf_->add_modify_callback(style_update_cb, this);
    
    prompt_pos_ = buf_->length(); // advance prompt pos
    insert_position(prompt_pos_);
    show_insert_position();
}

void TerminalPane::evaluateCommand() {
    int len = buf_->length() - prompt_pos_;
    if (len < 0) len = 0;
    char* cmd = buf_->text_range(prompt_pos_, buf_->length());
    
    // Add to history
    std::string s(cmd);
    if (!s.empty()) {
        history_.push_back(s);
        history_index_ = history_.size();
    }
    
    // Echo newline
    buf_->remove_modify_callback(style_update_cb, this);
    buf_->insert(buf_->length(), "\n");
    buf_->add_modify_callback(style_update_cb, this);
    
    // Evaluate via engine
    hazel_ctx_t ctx;
    ctx.app = app_;
    ctx.insert_pos = buf_->length();
    ctx.at_bottom = true;
    ctx.is_terminal = true;
    
    app_->evaluateCommand(cmd, &ctx);
    
    free(cmd);
    
    printPrompt();
}

int TerminalPane::handle(int event) {
    if (event == FL_KEYBOARD || event == FL_SHORTCUT) {
        int key = Fl::event_key();
        int state = Fl::event_state();
        
        // Hide terminal on Ctrl+~
        if (key == '`' && (state & FL_COMMAND)) {
            // we will let HazelApp intercept this, so return 0
            return 0;
        }
        
        if (key == FL_Enter || key == FL_KP_Enter) {
            evaluateCommand();
            return 1;
        } else if (key == FL_Up) {
            if (history_index_ > 0) {
                history_index_--;
                buf_->remove(prompt_pos_, buf_->length());
                buf_->insert(prompt_pos_, history_[history_index_].c_str());
                insert_position(buf_->length());
            }
            return 1;
        } else if (key == FL_Down) {
            if (history_index_ + 1 < history_.size()) {
                history_index_++;
                buf_->remove(prompt_pos_, buf_->length());
                buf_->insert(prompt_pos_, history_[history_index_].c_str());
                insert_position(buf_->length());
            } else if (history_index_ + 1 == history_.size()) {
                history_index_++;
                buf_->remove(prompt_pos_, buf_->length());
                insert_position(buf_->length());
            }
            return 1;
        } else if (key == FL_BackSpace) {
            if (insert_position() <= prompt_pos_) return 1;
        } else if (key == FL_Left) {
            if (insert_position() <= prompt_pos_) return 1;
        } else if (key == FL_Home) {
            insert_position(prompt_pos_);
            show_insert_position();
            return 1;
        }
    }
    
    return Fl_Text_Editor::handle(event);
}
