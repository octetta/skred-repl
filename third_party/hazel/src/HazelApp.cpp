#include "HazelApp.h"
#include "Preferences.h"
#include <FL/Fl.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <iostream>
#include <string.h>



static void style_update_cb(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg) {
    HazelApp* app = (HazelApp*)cbArg;
    Fl_Text_Buffer* style_buf = app->getStyleBuffer();
    
    if (nInserted > 0 || nDeleted > 0) {
        app->setDirty(true);
        int current = app->getHighestModifiedPos();
        if (current == -1 || pos < current) {
            app->setHighestModifiedPos(pos);
        }
    }
    
    if (nInserted > 0) {
        char target_style = 'A';
        char prev = (pos > 0) ? app->getStyleAt(pos - 1) : '\0';
        char curr = app->getStyleAt(pos);
        
        if (app->getPendingStyle() != 0) {
            target_style = app->getPendingStyle();
            app->setPendingStyle(0);
        } else {
            if (prev == 'A' || prev == 'D') target_style = prev;
            else if (curr == 'A' || curr == 'D') target_style = curr;
        }
        
        std::string styles(nInserted, target_style);
        style_buf->replace(pos, pos + nDeleted, styles.c_str());
    } else if (nDeleted > 0) {
        style_buf->remove(pos, pos + nDeleted);
    }
}

HazelEditor::HazelEditor(int x, int y, int w, int h, HazelApp* app) 
    : Fl_Text_Editor(x, y, w, h), app_(app) {
    linenumber_width(40);
    linenumber_fgcolor(FL_DARK3);
    linenumber_bgcolor(FL_LIGHT2);
}

int HazelEditor::handle(int event) {
    if (event == FL_KEYBOARD) {
        if (Fl::event_key() == FL_Up || Fl::event_key() == FL_Down || Fl::event_key() == FL_Left || Fl::event_key() == FL_Right) {
            app_->setPendingStyle(0);
        }
        int key = Fl::event_key();
        
        if (key == 'q' && (Fl::event_state() & FL_COMMAND)) {
            app_->tryQuit();
            return 1;
        } else if (key == 'r' && (Fl::event_state() & FL_COMMAND)) {
            app_->startRunAll();
            return 1;
        } else if (key == 's' && (Fl::event_state() & FL_COMMAND)) {
            app_->saveFile();
            return 1;
        } else if (key == 'o' && (Fl::event_state() & FL_COMMAND)) {
            app_->openFile();
            return 1;
        } else if (key == 'd' && (Fl::event_state() & FL_COMMAND)) {
            FILE* fp = fopen("dump.txt", "w");
            fprintf(fp, "--- TEXT ---\n%s\n--- STYLE ---\n%s\n", buffer()->text(), app_->getStyleBuffer()->text());
            fclose(fp);
            return 1;
        }
        
        // Convert to Markdown
        if (key == 'm' && (Fl::event_state() & FL_COMMAND)) {
            int pos = insert_position();
            char style = app_->getStyleAt(pos);
            if (style != 'A' && pos > 0 && app_->getStyleAt(pos - 1) == 'A' && buffer()->char_at(pos - 1) != '\n') {
                pos = pos - 1;
                style = 'A';
            }
            if (style == 'A') {
                int start = pos;
                while (start > 0 && app_->getStyleAt(start - 1) == style) start--;
                int end = pos;
                while (end < buffer()->length() - 1 && app_->getStyleAt(end + 1) == style) end++;
                if (end < buffer()->length()) end++;
                std::string new_styles(end - start, 'D');
                app_->getStyleBuffer()->replace(start, end, new_styles.c_str());
                this->redisplay_range(start, end);
            } else {
                app_->setPendingStyle('D');
                this->redraw();
            }
            return 1;
        }
        
        // Convert to Code
        if (key == 'y' && (Fl::event_state() & FL_COMMAND)) {
            int pos = insert_position();
            char style = app_->getStyleAt(pos);
            if (style != 'D' && pos > 0 && app_->getStyleAt(pos - 1) == 'D' && buffer()->char_at(pos - 1) != '\n') {
                pos = pos - 1;
                style = 'D';
            }
            if (style == 'D') {
                int start = pos;
                while (start > 0 && app_->getStyleAt(start - 1) == style) start--;
                int end = pos;
                while (end < buffer()->length() - 1 && app_->getStyleAt(end + 1) == style) end++;
                if (end < buffer()->length()) end++;
                std::string new_styles(end - start, 'A');
                app_->getStyleBuffer()->replace(start, end, new_styles.c_str());
                this->redisplay_range(start, end);
            } else {
                app_->setPendingStyle('A');
                this->redraw();
            }
            return 1;
        }
        
        // Preferences
        if (key == ',' && (Fl::event_state() & FL_COMMAND)) {
            PreferencesWindow prefs(app_->getConfig());
            std::string font;
            int theme = 0;
            if (prefs.run(font, theme)) {
                hazel_config_t cfg = app_->getConfig();
                if (!font.empty()) {
                    Fl::set_font(FL_FREE_FONT, font.c_str());
                    cfg.font = FL_FREE_FONT;
                }
                
                if (theme == 0) { // Light
                    cfg.text_fg = FL_BLACK;
                    cfg.input_bg = FL_WHITE;
                    cfg.output_bg = fl_rgb_color(245, 245, 250);
                    cfg.error_bg = fl_rgb_color(255, 235, 235);
                    cfg.markdown_bg = fl_rgb_color(245, 255, 245);
                } else if (theme == 1) { // Dark
                    cfg.text_fg = fl_rgb_color(220, 220, 220);
                    cfg.input_bg = fl_rgb_color(25, 25, 30);
                    cfg.output_bg = fl_rgb_color(15, 15, 20);
                    cfg.error_bg = fl_rgb_color(40, 10, 10);
                    cfg.markdown_bg = fl_rgb_color(20, 30, 25);
                }
                app_->setConfig(&cfg);
            }
            return 1;
        }
        
        // Evaluate Block
        if ((key == FL_Enter || key == FL_KP_Enter) && (Fl::event_state() & FL_COMMAND)) {
            app_->evaluateCurrentBlock();
            return 1;
        }
        
        // Prevent modification of output cells (B = Output, C = Error)
        bool is_modifying = (key == FL_BackSpace || key == FL_Delete || key == FL_Enter || key == FL_KP_Enter || (key >= 0x20 && key <= 0xff && !(Fl::event_state() & FL_CTRL) && !(Fl::event_state() & FL_ALT)) || ((key == 'v' || key == 'x') && (Fl::event_state() & FL_COMMAND)));
        if (is_modifying) {
            if (buffer()->selected()) {
                int start, end;
                if (buffer()->selection_position(&start, &end)) {
                    for (int i = start; i < end; i++) {
                        char s = app_->getStyleAt(i);
                        if (app_->isOutputStyle(s)) return 1;
                    }
                }
            } else {
                int pos = insert_position();
                if (key == FL_BackSpace) {
                    if (pos > 0) {
                        char s = app_->getStyleAt(pos - 1);
                        if (app_->isOutputStyle(s)) return 1;
                    }
                } else if (key == FL_Delete) {
                    if (pos < buffer()->length()) {
                        char s = app_->getStyleAt(pos);
                        if (app_->isOutputStyle(s)) return 1;
                    }
                } else {
                    if (pos > 0) {
                        char prev = app_->getStyleAt(pos - 1);
                        char curr = app_->getStyleAt(pos);
                        if (app_->isOutputStyle(prev) && app_->isOutputStyle(curr)) return 1;
                    } else {
                        char curr = app_->getStyleAt(pos);
                        if (app_->isOutputStyle(curr)) return 1;
                    }
                }
            }
        }
        
        // Bash-style keybindings
        if ((Fl::event_state() & FL_COMMAND)) {
            if (key == 'a') {
                insert_position(buffer()->line_start(insert_position()));
                show_insert_position();
                return 1;
            } else if (key == 'e') {
                insert_position(buffer()->line_end(insert_position()));
                show_insert_position();
                return 1;
            } else if (key == 'k') {
                int pos = insert_position();
                int end = buffer()->line_end(pos);
                if (pos == end) buffer()->remove(pos, pos + 1);
                else buffer()->remove(pos, end);
                return 1;
            } else if (key == 'f') {
                if (insert_position() < buffer()->length()) insert_position(insert_position() + 1);
                show_insert_position();
                return 1;
            } else if (key == 'b') {
                if (insert_position() > 0) insert_position(insert_position() - 1);
                show_insert_position();
                return 1;
            } else if (key == 'p') {
                kf_up(0, this);
                return 1;
            } else if (key == 'n') {
                kf_down(0, this);
                return 1;
            }
        }
    }
    
    int r = Fl_Text_Editor::handle(event);
    app_->updateStatusBar();
    return r;
}

HazelApp::HazelApp(const char* title, hazel_eval_cb_t cb, void* user_data) 
    : eval_cb_(cb), user_data_(user_data) {
    
    is_dirty_ = false;
    prefs_ = new Fl_Preferences(Fl_Preferences::USER, "octetta", "hazel");
    win_ = new Fl_Double_Window(800, 600, title);
    buffer_ = new Fl_Text_Buffer();
    style_buffer_ = new Fl_Text_Buffer();
    
    config_.font = FL_COURIER;
    config_.font_size = 14;
    config_.input_bg = FL_WHITE;
    config_.output_bg = fl_rgb_color(240, 240, 245);
    config_.error_bg = fl_rgb_color(255, 230, 230);
    config_.markdown_bg = fl_rgb_color(240, 255, 240);
    config_.text_fg = FL_BLACK;
    
    applyConfig();
    
    editor_ = new HazelEditor(0, 0, 800, 575, this);
    editor_->buffer(buffer_);
    editor_->box(FL_FLAT_BOX);
    editor_->highlight_data(style_buffer_, styletable_, next_style_index_, 'A', 0, 0);
    
    status_bar_ = new Fl_Box(0, 575, 800, 25, "");
    status_bar_->box(FL_FLAT_BOX);
    status_bar_->color(FL_LIGHT2);
    status_bar_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    win_->resizable(editor_);
    win_->callback([](Fl_Widget*, void* v){ ((HazelApp*)v)->tryQuit(); }, this);
    win_->end();
    
    // Setup initial notebook state
    const char* startup = "// Welcome to Hazel Notebook.\n// Press Enter to edit code. Press Ctrl+Enter to evaluate.\n// Press Esc to return to Command mode.\n";
    buffer_->text(startup);
    std::string start_styles(strlen(startup), 'D');
    style_buffer_->text(start_styles.c_str());
    
    buffer_->append("\n"); // Fresh input cell
    style_buffer_->append("A");
    
    buffer_->add_modify_callback(style_update_cb, this);
    
    editor_->insert_position(buffer_->length());
    highest_modified_pos_ = 0;
    
}

HazelApp::~HazelApp() {
    delete prefs_;
    delete win_;
    delete buffer_;
    delete style_buffer_;
}

int HazelApp::run() {
    win_->show();
    return Fl::run();
}

void HazelApp::loadFile(const char* filepath) {
    if (config_.on_open) {
        if (config_.on_open((hazel_app_t*)this, filepath, user_data_)) {
            editor_->insert_position(0);
            editor_->show_insert_position();
            return;
        }
    }

    FILE* f = fopen(filepath, "r");
    if (!f) return;
    
    buffer_->remove_modify_callback(style_update_cb, this);
    buffer_->text("");
    style_buffer_->text("");
    
    char line[2048];
    char current_style = 'D'; // Default to Markdown
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "```hazel", 8) == 0) {
            current_style = 'A';
            continue;
        } else if (strncmp(line, "```output", 9) == 0) {
            current_style = 'B';
            continue;
        } else if (strncmp(line, "```", 3) == 0) {
            current_style = 'D';
            continue;
        }
        
        int pos = buffer_->length();
        buffer_->insert(pos, line);
        int len = strlen(line);
        std::string styles(len, current_style);
        style_buffer_->insert(pos, styles.c_str());
    }
    fclose(f);
    buffer_->add_modify_callback(style_update_cb, this);
    
    highest_modified_pos_ = 0;
    current_filepath_ = filepath;
    setDirty(false);
    editor_->insert_position(0);
}

void HazelApp::openFile() {
    Fl_Native_File_Chooser fnfc;
    fnfc.title("Open Notebook");
    fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    fnfc.filter("Notebook Files\t*.{md,sk}\nMarkdown\t*.md\nSkred Script\t*.sk\nAll Files\t*");
    if (fnfc.show() == 0) {
        loadFile(fnfc.filename());
    }
}
void HazelApp::saveFileAs(const char* filepath) {
    if (config_.on_save) {
        if (config_.on_save((hazel_app_t*)this, filepath, user_data_)) return;
    }

    std::string path(filepath);
    size_t last_slash = path.find_last_of("/\\");
    size_t last_dot = path.find_last_of(".");
    if (last_dot == std::string::npos || (last_slash != std::string::npos && last_dot < last_slash)) {
        path += ".md";
    }
    
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return;
    
    char current_style = 0;
    int block_start = 0;
    
    auto flush_block = [&](int end_pos) {
        if (block_start >= end_pos) return;
        char* text = buffer_->text_range(block_start, end_pos);
        int len = strlen(text);
        
        if (current_style == 'A') {
            fprintf(f, "```hazel\n%s", text);
            if (len == 0 || text[len-1] != '\n') fprintf(f, "\n");
            fprintf(f, "```\n");
        } else if (isOutputStyle(current_style)) {
            fprintf(f, "```output\n%s", text);
            if (len == 0 || text[len-1] != '\n') fprintf(f, "\n");
            fprintf(f, "```\n");
        } else if (current_style == 'D') {
            fprintf(f, "%s", text);
            if (len == 0 || text[len-1] != '\n') fprintf(f, "\n");
        }
        
        free(text);
    };
    
    for (int i = 0; i < buffer_->length(); i++) {
        char s = style_buffer_->char_at(i);
        if (s == 'C') s = 'B';
        if (s != current_style) {
            flush_block(i);
            current_style = s;
            block_start = i;
        }
    }
    flush_block(buffer_->length());
    fclose(f);
    current_filepath_ = path;
    setDirty(false);
}

void HazelApp::saveFile() {
    Fl_Native_File_Chooser fnfc;
    fnfc.title("Save Notebook");
    fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    fnfc.filter("Notebook Files\t*.{md,sk}\nMarkdown\t*.md\nSkred Script\t*.sk\nAll Files\t*");
    if (fnfc.show() == 0) {
        saveFileAs(fnfc.filename());
    }
}

void HazelApp::evaluateCurrentBlock() {
    if (!eval_cb_) return;
    
    int pos = editor_->insert_position();
    char style = getStyleAt(pos);
    
    if (style != 'A' && style != 'D' && pos > 0) {
        char left = getStyleAt(pos - 1);
        if (left == 'A' || left == 'D') {
            pos = pos - 1;
            style = left;
        }
    }
    
    if (style != 'A' && style != 'D') return; 
    
    int start = pos;
    while (start > 0 && style_buffer_->char_at(start - 1) == style) start--;
    int end = pos;
    while (end < buffer_->length() - 1 && style_buffer_->char_at(end + 1) == style) end++;
    if (end < buffer_->length()) end++;
    
    if (style == 'D') {
        int next_pos = end;
        if (next_pos >= buffer_->length()) {
            buffer_->remove_modify_callback(style_update_cb, this);
            while (end > start && buffer_->char_at(end - 1) == '\n') {
                buffer_->remove(end - 1, end);
                style_buffer_->remove(end - 1, end);
                end--;
            }
            buffer_->insert(end, "\n");
            style_buffer_->insert(end, "A");
            buffer_->add_modify_callback(style_update_cb, this);
            next_pos = end + 1;
        }
        
        editor_->insert_position(next_pos);
        editor_->show_insert_position();
        
        if (run_all_pending_) {
            Fl::add_timeout(0.01, [](void* d) {
                ((HazelApp*)d)->evaluateCurrentBlock();
            }, this);
        }
        return;
    }
    // Check if there is an existing output block immediately following this input block.
    // An output block consists of contiguous 'B' and 'C' characters.
    int output_end = end;
    while (output_end < buffer_->length()) {
        char s = style_buffer_->char_at(output_end);
        if (isOutputStyle(s)) {
            output_end++;
        } else {
            break;
        }
    }
    
    if (output_end > end) {
        buffer_->remove_modify_callback(style_update_cb, this);
        buffer_->remove(end, output_end);
        style_buffer_->remove(end, output_end);
        buffer_->add_modify_callback(style_update_cb, this);
    }
    
    if (highest_modified_pos_ != -1 && highest_modified_pos_ >= start && highest_modified_pos_ <= end) {
        highest_modified_pos_ = end;
    }
    char* input = buffer_->text_range(start, end);
    
    // Force exactly one newline after the input block
    buffer_->remove_modify_callback(style_update_cb, this);
    while (end > start && buffer_->char_at(end - 1) == '\n') {
        buffer_->remove(end - 1, end);
        style_buffer_->remove(end - 1, end);
        end--;
    }
    buffer_->insert(end, "\n");
    style_buffer_->insert(end, "A");
    buffer_->add_modify_callback(style_update_cb, this);
    end++;
    
    bool at_bottom = (end >= buffer_->length());
    
    hazel_ctx_t* ctx = new hazel_ctx_t();
    ctx->app = this;
    ctx->insert_pos = end;
    ctx->at_bottom = at_bottom;
    
    printf("Calling eval_cb_! input=%s\n", input);
    eval_cb_(input, ctx, user_data_);
    free(input);
}

// Very basic 256 color map approximation
static unsigned int map_256_to_rgb(int code) {
    if (code < 16) {
        // Standard ANSI 16
        unsigned int colors[16] = {
            0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0xc0c0c0,
            0x808080, 0xff0000, 0x00ff00, 0xffff00, 0x0000ff, 0xff00ff, 0x00ffff, 0xffffff
        };
        return colors[code];
    } else if (code < 232) {
        // 6x6x6 color cube
        code -= 16;
        int b = code % 6;
        int g = (code / 6) % 6;
        int r = (code / 36) % 6;
        return fl_rgb_color(r ? r * 40 + 55 : 0, g ? g * 40 + 55 : 0, b ? b * 40 + 55 : 0);
    } else {
        // 24 grayscale
        int gray = (code - 232) * 10 + 8;
        return fl_rgb_color(gray, gray, gray);
    }
}

char HazelApp::getAnsiStyle(unsigned int fg, unsigned int bg, bool is_error) {
    for (int i = 4; i < next_style_index_; i++) {
        if (styletable_[i].color == (Fl_Color)fg && styletable_[i].bgcolor == (Fl_Color)bg) {
            return (char)('A' + i);
        }
    }
    
    if (next_style_index_ >= 250) {
        return is_error ? 'C' : 'B';
    }
    
    int i = next_style_index_++;
    styletable_[i] = { (Fl_Color)fg, config_.font, config_.font_size, Fl_Text_Display::ATTR_BGCOLOR_EXT, (Fl_Color)bg };
    
    editor_->highlight_data(style_buffer_, styletable_, next_style_index_, 'A', 0, 0);
    return (char)('A' + i);
}

void HazelApp::appendOutput(int insert_pos, const char* text, int is_error) {
    if (!text || !buffer_ || !style_buffer_) return;
    
    std::string clean_text;
    std::string clean_styles;
    
    char current_style = is_error ? 'C' : 'B';
    unsigned int current_fg = is_error ? config_.error_bg : config_.text_fg;
    unsigned int current_bg = is_error ? config_.error_bg : config_.output_bg;
    
    const char* src = text;
    while (*src) {
        if (*src == '\x1b' && *(src + 1) == '[') {
            src += 2;
            int codes[16] = {0};
            int num_codes = 0;
            
            while (*src && *src != 'm' && num_codes < 16) {
                if (*src >= '0' && *src <= '9') {
                    codes[num_codes] = codes[num_codes] * 10 + (*src - '0');
                } else if (*src == ';') {
                    num_codes++;
                }
                src++;
            }
            if (*src == 'm') {
                num_codes++;
                src++;
            }
            
            // Apply ANSI codes
            for (int i = 0; i < num_codes; i++) {
                int c = codes[i];
                if (c == 0) {
                    current_fg = config_.text_fg;
                    current_bg = is_error ? config_.error_bg : config_.output_bg;
                    current_style = is_error ? 'C' : 'B';
                } else if (c == 38 && i + 2 < num_codes && codes[i+1] == 5) {
                    current_fg = map_256_to_rgb(codes[i+2]);
                    current_style = getAnsiStyle(current_fg, current_bg, is_error);
                    i += 2;
                } else if (c == 48 && i + 2 < num_codes && codes[i+1] == 5) {
                    current_bg = map_256_to_rgb(codes[i+2]);
                    current_style = getAnsiStyle(current_fg, current_bg, is_error);
                    i += 2;
                } else if (c >= 30 && c <= 37) {
                    current_fg = map_256_to_rgb(c - 30);
                    current_style = getAnsiStyle(current_fg, current_bg, is_error);
                } else if (c >= 40 && c <= 47) {
                    current_bg = map_256_to_rgb(c - 40);
                    current_style = getAnsiStyle(current_fg, current_bg, is_error);
                }
            }
            continue;
        }
        
        clean_text += *src;
        clean_styles += current_style;
        src++;
    }
    
    if (clean_text.empty()) return;
    
    buffer_->remove_modify_callback(style_update_cb, this);
    buffer_->insert(insert_pos, clean_text.c_str());
    style_buffer_->insert(insert_pos, clean_styles.c_str());
    buffer_->add_modify_callback(style_update_cb, this);
    
    // Push the user's cursor to the end of the output so they are ready to type
    if (editor_->insert_position() <= insert_pos) {
        editor_->insert_position(insert_pos + clean_text.length());
        editor_->show_insert_position();
    }
}

char HazelApp::getStyleAt(int pos) {
    if (pos >= buffer_->length()) {
        return '\0'; // End of file has no style!
    }
    return style_buffer_->char_at(pos);
}

void HazelApp::startRunAll() {
    int start_pos = 0;
    if (highest_modified_pos_ != -1) {
        start_pos = highest_modified_pos_;
    }
    
    highest_modified_pos_ = 0;
    
    
    int block_start = start_pos;
    if (block_start < buffer_->length()) {
        char s = getStyleAt(block_start);
        if (s == 'A') {
            while (block_start > 0 && getStyleAt(block_start - 1) == 'A') block_start--;
        } else if (s == 'D') {
            while (block_start > 0 && getStyleAt(block_start - 1) == 'D') block_start--;
        }
    }
    
    editor_->insert_position(block_start);
    if (getStyleAt(block_start) != 'A') {
        int search = block_start;
        while (search < buffer_->length() && getStyleAt(search) != 'A') search++;
        editor_->insert_position(search);
    }
    
    highest_modified_pos_ = -1; // Reset since we are running!
    
    if (getStyleAt(editor_->insert_position()) == 'A') {
        run_all_pending_ = true;
        evaluateCurrentBlock();
    }
}

void HazelApp::finishEvaluation(hazel_ctx_t* ctx) {
    if (ctx->at_bottom) {
        
        editor_->insert_position(buffer_->length());
        editor_->show_insert_position();
    } else {
        highest_modified_pos_ = 0;
    
        int search = ctx->insert_pos;
        while (search < buffer_->length()) {
            char s = getStyleAt(search);
            if (s == 'A' || s == 'D') {
                editor_->insert_position(search);
                
                break;
            }
            search++;
        }
    }

    if (run_all_pending_) {
        int pos = editor_->insert_position();
        char style = getStyleAt(pos);
        
        if (style == 'A' || style == 'D') {
            int end = pos;
            while (end < buffer_->length() && getStyleAt(end) == style) end++;
            
            // A cell is considered empty if it contains just a trailing newline scaffold at the EOF.
            if (style == 'A' && end - pos <= 1 && end >= buffer_->length()) {
                run_all_pending_ = false;
                
                editor_->insert_position(buffer_->length());
            } else {
                Fl::add_timeout(0.01, [](void* d) {
                    ((HazelApp*)d)->evaluateCurrentBlock();
                }, this);
            }
        } else {
            run_all_pending_ = false;
        }
    }
}

void HazelEditor::draw() {
    Fl_Text_Editor::draw();
    
    int margin_x = this->x() + this->linenumber_width();
    int width = this->w() - this->linenumber_width();
    int height = this->textsize() + 6; 
    
    int y_start = this->y();
    int y_end = this->y() + this->h();
    
    auto getEffectiveStyleAt = [&](int p) -> char {
        if (p < 0) return 'A';
        int l_start = buffer()->line_start(p);
        int l_end = buffer()->line_end(p);
        if (l_start == l_end) {
            if (p == insert_position() && app_->getPendingStyle() != 0) return app_->getPendingStyle();
            char p_prev = (p > 0) ? app_->getStyleAt(p - 1) : '\0';
            char p_curr = app_->getStyleAt(p);
            if (p_prev == 'A' || p_prev == 'D') return p_prev;
            if (p_curr == 'A' || p_curr == 'D') return p_curr;
            return 'A';
        }
        char s = app_->getStyleAt(p);
        return s == 0 ? 'A' : s;
    };
    
    // Iterate over visible screen space to find empty lines
    for (int y = y_start; y < y_end; y += height) {
        int pos = xy_to_position(this->x() + this->linenumber_width(), y);
        if (pos < 0 || pos > buffer()->length()) continue;
        
        int line_start = buffer()->line_start(pos);
        int line_end = buffer()->line_end(pos);
        
        if (line_start == line_end) {
            char p = getEffectiveStyleAt(pos);
            if (p == 'D' || app_->isOutputStyle(p)) {
                int cx, cy;
                if (position_to_xy(pos, &cx, &cy)) {
                    if (p == 'D') fl_color(fl_rgb_color(240, 255, 240));
                    else if (p == 'B') fl_color(fl_rgb_color(240, 240, 245));
                    else if (p == 'C') fl_color(fl_rgb_color(255, 230, 230));
                    
                    fl_rectf(margin_x, cy, width, height);
                    
                    if (pos == insert_position()) {
                        fl_color(FL_BLACK);
                        fl_rectf(cx, cy, 2, height);
                    }
                }
            }
        }
    }
    
    // Draw Custom Cell Badges in Margin
    int m_width = 40;
    int m_x = this->x();
    fl_color(fl_rgb_color(230, 230, 230)); 
    fl_rectf(m_x, y_start, m_width, y_end - y_start);
    
    auto getBlockType = [&](char s) {
        if (s == 'D') return 1; // Markdown
        if (app_->isOutputStyle(s)) return 2; // Output
        return 0; // Code
    };
    
    fl_font(FL_HELVETICA_BOLD, 10);
    fl_color(fl_rgb_color(150, 150, 150));
    
    for (int y = y_start; y < y_end; y += height) {
        int pos = xy_to_position(this->x() + m_width, y);
        if (pos < 0 || pos > buffer()->length()) continue;
        
        int line_start = buffer()->line_start(pos);
        if (pos == line_start) { // First char of the line
            char curr = getEffectiveStyleAt(line_start);
            char prev = (line_start > 0) ? getEffectiveStyleAt(line_start - 1) : '\0';
            if (prev == 0) prev = 'A';
            
            int curr_type = getBlockType(curr);
            int prev_type = getBlockType(prev);
            
            if (line_start == 0 || curr_type != prev_type) {
                int block_idx = 1;
                char scan_curr = '\0';
                for (int i = 0; i < line_start; i++) {
                    char s = getEffectiveStyleAt(i);
                    int t = getBlockType(s);
                    if (t != getBlockType(scan_curr)) {
                        if (t == curr_type) block_idx++;
                        scan_curr = s;
                    }
                }
                
                char badge[16];
                if (curr_type == 0) snprintf(badge, sizeof(badge), "C%d", block_idx);
                else if (curr_type == 1) snprintf(badge, sizeof(badge), "M%d", block_idx);
                else snprintf(badge, sizeof(badge), "O%d", block_idx);
                
                int cx, cy;
                if (position_to_xy(line_start, &cx, &cy)) {
                    fl_draw(badge, m_x + 4, cy + height - 4);
                }
            }
        }
    }
    
    // Draw a subtle border separating margin from content
    fl_color(fl_rgb_color(210, 210, 210));
    fl_line(m_x + m_width - 1, y_start, m_x + m_width - 1, y_end);
}

void HazelApp::updateStatusBar() {
    int pos = editor_->insert_position();
    int line = buffer_->count_lines(0, pos) + 1;
    int line_start = buffer_->line_start(pos);
    int col = pos - line_start + 1;
    
    char style = getStyleAt(pos);
    if (pos == buffer_->length() && pos > 0 && buffer_->char_at(pos - 1) == '\n') {
        style = pending_style_ ? pending_style_ : 'A';
    } else if (pos == buffer_->length()) {
        style = pending_style_ ? pending_style_ : getStyleAt(pos - 1);
    }
    
    const char* mode = "Code";
    if (style == 'D') mode = "Markdown";
    else if (isOutputStyle(style)) mode = "Output";
    
    int block_idx = 0;
    char current_block = '\0';
    char target_block = isOutputStyle(style) ? 'B' : style;
    for (int i = 0; i < pos; i++) {
        char s = getStyleAt(i);
        if (isOutputStyle(s)) s = 'B';
        if (s != current_block) {
            if (s == target_block) block_idx++;
            current_block = s;
        }
    }
    if (block_idx == 0) block_idx = 1;
    if (pos == 0) block_idx = 1;

    char mode_with_idx[64];
    snprintf(mode_with_idx, sizeof(mode_with_idx), "%s#%d", mode, block_idx);
    
    const char* fname = current_filepath_.empty() ? "Untitled" : current_filepath_.c_str();
    const char* slash = strrchr(fname, '/');
    if (slash) fname = slash + 1;
    
    char status[512];
    snprintf(status, sizeof(status), " %s%s  |  Ln %d, Col %d  |  %s  |  ^, Pref  ^RET Eval  ^R RunAll  ^Y Code  ^U Mkdn  ^Q Quit", 
             fname, is_dirty_ ? "*" : "", line, col, mode_with_idx);
    
    if (!status_bar_->label() || strcmp(status, status_bar_->label()) != 0) {
        status_bar_->copy_label(status);
        status_bar_->redraw();
    }
}

void HazelApp::setDirty(bool dirty) {
    is_dirty_ = dirty;
    updateStatusBar();
}

bool HazelApp::checkSaveBeforeQuit() {
    if (!is_dirty_) return true;
    int r = fl_choice("You have unsaved changes. Quit anyway?", "Cancel", "Quit", 0);
    return r == 1;
}

void HazelApp::tryQuit() {
    if (checkSaveBeforeQuit()) {
        win_->hide();
    }
}

void HazelApp::applyConfig() {
    styletable_[0] = { (Fl_Color)config_.text_fg, config_.font, config_.font_size, Fl_Text_Display::ATTR_BGCOLOR_EXT, (Fl_Color)config_.input_bg };
    styletable_[1] = { (Fl_Color)config_.text_fg, config_.font, config_.font_size, Fl_Text_Display::ATTR_BGCOLOR_EXT, (Fl_Color)config_.output_bg };
    styletable_[2] = { FL_DARK_RED, config_.font | FL_BOLD, config_.font_size, Fl_Text_Display::ATTR_BGCOLOR_EXT, (Fl_Color)config_.error_bg };
    styletable_[3] = { FL_DARK_GREEN, config_.font | FL_ITALIC, config_.font_size, Fl_Text_Display::ATTR_BGCOLOR_EXT, (Fl_Color)config_.markdown_bg };
}

void HazelApp::setConfig(const hazel_config_t* config) {
    if (!config) return;
    config_ = *config;
    applyConfig();
    if (editor_) {
        editor_->highlight_data(style_buffer_, styletable_, next_style_index_, 'A', 0, 0);
        editor_->textfont(config_.font);
        editor_->textsize(config_.font_size);
        editor_->redraw();
    }
}

void HazelApp::clear() {
    buffer_->remove_modify_callback(style_update_cb, this);
    buffer_->text("");
    style_buffer_->text("");
    buffer_->add_modify_callback(style_update_cb, this);
    highest_modified_pos_ = 0;
    setDirty(false);
}

void HazelApp::appendBlock(char style, const char* text) {
    if (!text || strlen(text) == 0) return;
    buffer_->remove_modify_callback(style_update_cb, this);
    int pos = buffer_->length();
    buffer_->insert(pos, text);
    std::string styles(strlen(text), style);
    style_buffer_->insert(pos, styles.c_str());
    buffer_->add_modify_callback(style_update_cb, this);
}

const char* HazelApp::getText() const {
    return buffer_->text();
}

const char* HazelApp::getStyles() const {
    return style_buffer_->text();
}

void HazelApp::setFilepath(const char* path) {
    current_filepath_ = path ? path : "";
    updateStatusBar();
}
