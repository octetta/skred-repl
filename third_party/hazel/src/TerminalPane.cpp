#include "TerminalPane.h"
#include "HazelApp.h"
#include "hazel/hazel.h"
#include <iostream>



void TerminalPane::style_update_cb(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg) {
    TerminalPane* term = (TerminalPane*)cbArg;
    
    if (nInserted == 0 && nDeleted == 0) {
        term->style_buf_->unselect();
        return;
    }
    
    if (nDeleted > 0) {
        term->style_buf_->remove(pos, pos + nDeleted);
    }
    
    if (nInserted > 0) {
        std::string s(nInserted, 'A');
        term->style_buf_->insert(pos, s.c_str());
    }
    
    term->style_buf_->unselect();
}

TerminalPane::TerminalPane(int X, int Y, int W, int H, HazelApp* app) 
    : Fl_Text_Editor(X, Y, W, H), app_(app), history_index_(0), prompt_pos_(0) {
    
    buf_ = new Fl_Text_Buffer();
    style_buf_ = new Fl_Text_Buffer();
    
    this->buffer(buf_);
    this->box(FL_FLAT_BOX);
    this->color(app->config_.input_bg); // Match code cells
    this->textcolor(app->config_.text_fg);
    this->textfont(app->config_.font);
    this->textsize(app->config_.font_size);
    this->cursor_color(app->config_.text_fg);
    this->cursor_style(Fl_Text_Display::SIMPLE_CURSOR);
    
    this->highlight_data(style_buf_, app_->styletable_, app_->next_style_index_, 'A', 0, 0);
    buf_->add_modify_callback(style_update_cb, this);
    
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
    std::string s(strlen(prompt), 'A');
    style_buf_->insert(p, s.c_str());
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
    
    std::string clean_text = text;
    if (clean_text.back() != '\n') clean_text += '\n';
    
    std::string s(clean_text.length(), is_error ? 'C' : 'B');
    style_buf_->insert(p, s.c_str());
    buf_->insert(p, clean_text.c_str());
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
    int p = buf_->length();
    style_buf_->insert(p, "A");
    buf_->insert(p, "\n");
    buf_->add_modify_callback(style_update_cb, this);
    
    // Evaluate via engine
    hazel_ctx_t* ctx = new hazel_ctx_t();
    ctx->app = app_;
    ctx->insert_pos = buf_->length();
    ctx->at_bottom = true;
    ctx->is_terminal = true;
    
    app_->evaluateCommand(cmd, ctx);
    
    free(cmd);
    
    printPrompt();
}

int TerminalPane::handle(int event) {
    if (event == FL_KEYBOARD || event == FL_SHORTCUT) {
        int key = Fl::event_key();
        int state = Fl::event_state();
        
        // Hide terminal on Ctrl+~
        if ((key == '`' || key == '~') && (state & FL_COMMAND)) {
            app_->toggleTerminal();
            return 1;
        }
        
        if (key == FL_Escape) {
            app_->getEditor()->take_focus();
            return 1;
        } else if (key == FL_Enter || key == FL_KP_Enter) {
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

void TerminalPane::draw() {
    Fl_Text_Editor::draw();
    
    if (mCursorOn && Fl::focus() == this) {
        int pos = insert_position();
        int cx, cy;
        if (position_to_xy(pos, &cx, &cy)) {
            char c = (pos < buffer()->length()) ? buffer()->char_at(pos) : '\0';
            
            fl_font(textfont(), textsize());
            int c_width = fl_width("W");
            
            fl_color(FL_BLACK);
            fl_rectf(cx, cy, c_width, mMaxsize);
            
            if (c != '\n' && c != '\0' && c != '\r') {
                fl_color(FL_WHITE);
                char s[2] = {c, '\0'};
                fl_draw(s, cx, cy + mMaxsize - fl_descent());
            }
        }
    }
}
