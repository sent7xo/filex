#define CREATE_STRING(var, size, format, ...) char var[size]; \
                                              snprintf(var, size, \
                                                       format, __VA_ARGS__);
bool win32_file_exists (char *path){
    DWORD attrib = GetFileAttributes(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

// @analyze: use fuzzy finder library?
bool win32_find_file (char *buffer, size_t buffer_size, char *folder_path, char *filename)
{
    bool result = false;

    // @note: depth 0 is folder_path
    constexpr int max_depth = 4;
    int last_depth = -1;
    int depth = 0;

    // @note: dir is a stack of currently and previously used dirs
    constexpr int dir_name_size = PATH_MAX_SIZE;
    char dir[max_depth][dir_name_size];

    while (true) {
        char full_dir_path[MAX_PATH];

        strcpy_s(full_dir_path, folder_path);
        for (int i = 0; i < depth - 1; i++) {
            strcat_s(full_dir_path, dir[i]);
        }

        // @note: when we first open a folder, check
        // if the file we're looking for exists
        if (depth > last_depth) {
            char file_path[MAX_PATH];

            strcpy_s(file_path, full_dir_path);
            if (depth > 0) strcat_s(file_path, dir[depth - 1]);
            strcat_s(file_path, filename);

            if (win32_file_exists(file_path)) {
                strcpy_s(buffer, buffer_size, file_path);
                result = true;
                break;
            }
        }

        // @note: prepare to find folder
        if (depth == max_depth) {
            last_depth = depth;
            depth--;
        } else if (depth > 0) {
            strcat_s(full_dir_path, dir[depth - 1]);
        }
        strcat_s(full_dir_path, MAX_PATH, "*");

        bool found_next_dir = false;

        WIN32_FIND_DATAA file_data;
        HANDLE file = FindFirstFile(full_dir_path, &file_data);
        if (file != INVALID_HANDLE_VALUE) {
            defer { FindClose(file); };

            bool found_prev_dir = false;
            do {
                if (!(file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                if (strcmp(file_data.cFileName, ".") == 0 ||
                    strcmp(file_data.cFileName, "..") == 0 ||
                    stricmp(file_data.cFileName, "$RECYCLE.BIN") == 0 ||
                    stricmp(file_data.cFileName, "System Volume Information") == 0) continue;

                // @note: sadly, skip file name bigger than we can handle :(
                if (strlen(file_data.cFileName) > dir_name_size) continue;

                // @note: find the previous folder we were in, then find
                // the next folder to open
                if (depth < last_depth && !found_prev_dir) {
                    char temp[dir_name_size];
                    char *ctx;
                    strcpy_s(temp, dir[depth]);
                    strtok_s(temp, "\\", &ctx);
                    if (strcmp(temp, file_data.cFileName) == 0) {
                        found_prev_dir = true;
                    }
                    continue;
                }

                strcpy_s(dir[depth], file_data.cFileName);
                strcat_s(dir[depth], dir_name_size, "\\");
                last_depth = depth;
                depth++;
                found_next_dir = true;
                break;
            } while(FindNextFile(file, &file_data));
        }

        // @note: go up a dir
        if (!found_next_dir) {
            last_depth = depth;
            depth--;
        }

        if (depth < 0) {
            break;
        }
    }

    return result;
}

bool open_file_dialog (char path[MAX_PATH])
{
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    // ofn.lpstrFilter = "Images (*.png, *.jpg)\0*.png\0";
    ofn.lpstrFilter = NULL;
    ofn.Flags = OFN_FILEMUSTEXIST;

    return GetOpenFileName(&ofn) == TRUE;
}

ImTextureID get_file_tex (App_State *state, char *extension)
{
    ImTextureID id = state->tex_file.id;

    if (strcmp(extension, "exe") == 0) {
        id = state->tex_file_exe.id;
    } else if (strcmp(extension, "txt") == 0) {
        id = state->tex_file_txt.id;
    } else if (strcmp(extension, "mp3") == 0) {
        id = state->tex_file_mp3.id;
    } else if (strcmp(extension, "mp4") == 0) {
        id = state->tex_file_mp4.id;
    } else if (strcmp(extension, "png") == 0 ||
               strcmp(extension, "jpg") == 0 ||
               strcmp(extension, "jpeg") == 0) {
        id = state->tex_file_image.id;
    } else if (strcmp(extension, "zip") == 0) {
        id = state->tex_file_zip.id;
    } else if (strcmp(extension, "docx") == 0) {
        id = state->tex_file_word.id;
    }

    return id;
}

void text_centered (char *text, ImVec2 p, ImVec2 s, bool clip = true)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    char *text_end = text + strlen(text);

    const char *cursor = text;
    const char *word_wrap_eol = NULL;

    ImVec4 clipping = ImVec4(p.x,
                             p.y,
                             p.x + s.x,
                             p.y + s.y);
    // if (clip) {
    //     draw_list->PushClipRect(ImVec2(p.x, p.y), ImVec2(p.x + s.x, p.y + 10), true);
    // }

    int line = 0;
    while (cursor < text_end) {
        if (line > 5) break;
        word_wrap_eol = g.Font->ImFont::CalcWordWrapPositionA(1,
                                                              cursor,
                                                              text_end,
                                                              s.x);
        if (*cursor == ' ') {
            cursor++;
        }
        if (word_wrap_eol == cursor) {
            word_wrap_eol++;
        }

        float offset = (s.x - ImGui::CalcTextSize(cursor, word_wrap_eol, true, s.x).x) / 2;
        draw_list->AddText(g.Font,
                           g.FontSize,
                           ImVec2(p.x + offset, p.y + line * g.FontSize),
                           ImGui::GetColorU32(ImGuiCol_Text),
                           cursor,
                           word_wrap_eol,
                           s.x,
                           clip ? &clipping : NULL);

        cursor = word_wrap_eol;
        line++;
    }

    // if (clip) {
    //     ImGui::PopClipRect();
    // }
}

Texture get_texture(char *path)
{
    Texture tex;

    int channels = 0;
    uchar *image = stbi_load(path, &tex.width, &tex.height, &channels, 0);
    if (!image) {
        printf("File doesn't exists: %s\n", path);
        assert(false);
    }

    int type = GL_RGBA;
    if (channels == 3) {
        type = GL_RGB;
    }

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, type, tex.width, tex.height, 0, type, GL_UNSIGNED_BYTE, image);

    stbi_image_free(image);

    tex.id = (ImTextureID)(intptr_t)id;

    return tex;
}

void change_wallpaper (App_State *state)
{

}

Texture get_app_texture (App_State *state, char *path)
{
    char temp[PATH_MAX_SIZE];
    snprintf(temp, PATH_MAX_SIZE, "%s\\data\\%s", state->exe_path, path);
    return get_texture(temp);
}

// @note: "C:\\\\test\\\\\" -> "C:\test\"
void remove_multiple_slashes (char *path)
{
    char *cursor = path;
    char *next_cursor = path;
    bool slash = false;
    while (*next_cursor) {
        if (!slash) {
            if (*next_cursor == '\\') {
                slash = true;
            } else {
                *(cursor++) = *next_cursor;
            }
        } else {
            if (*next_cursor != '\\') {
                *(cursor++) = '\\';
                *(cursor++) = *next_cursor;
                slash = false;
            }
        }

        next_cursor++;
    }
    if (slash) {
        *(cursor++) = '\\';
    }
    *(cursor++) = '\0';
}

bool file_button (ImTextureID id, char *text, ImVec2 image_size)
{
    bool result = false;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 padding = style.FramePadding;
    ImVec2 text_size = ImGui::CalcTextSize(text, NULL, true, image_size.x - padding.x * 2);
    ImVec2 button_size = ImVec2(image_size.x, image_size.y + (text_size.y < 30 ? text_size.y : 30));

    result = ImGui::InvisibleButton(text, button_size + padding * 2);

    bool hover = ImGui::IsItemHovered() &&
                 (!ImGui::IsAnyItemActive() || ImGui::IsItemActive());
    bool click = ImGui::IsItemClicked(0);
    ImU32 col = ImGui::GetColorU32((hover && click) ? ImGuiCol_ButtonActive :
                                   hover ? ImGuiCol_ButtonHovered :
                                   ImGuiCol_Button);

    if (hover) {
        ImVec2 min = draw_list->GetClipRectMin();
        ImVec2 max = draw_list->GetClipRectMax();

        // @note: the size and location of this window doesn't matter,
        // we just need its draw_list
        ImGui::SetNextWindowFocus();
        ImGui::SetNextWindowSize(ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(0);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoFocusOnAppearing;
        ImGui::Begin("Folder Name Hover", NULL, flags);
        draw_list = ImGui::GetWindowDrawList();
        draw_list->PushClipRect(min, max);

        // @note: if the bottom of the hover is clipped, offset it
        float hover_bottom = p.y + image_size.y + text_size.y;
        if (hover_bottom > max.y) {
            p.y -= hover_bottom - max.y + padding.y * 2;
        }
    }

    draw_list->AddRectFilled(p,
                             ImVec2(p.x + button_size.x + padding.x * 2,
                                    p.y + (hover ? image_size.y + text_size.y : button_size.y) + padding.y * 2),
                             col,
                             ImClamp((float)ImMin(padding.x, padding.y), 0.0f, g.Style.FrameRounding));

    float offset = padding.x;
    draw_list->AddImage(id,
                        p + padding + ImVec2(offset, 0),
                        p + image_size - padding + ImVec2(offset, 0));
    ImVec4 clipping = ImVec4(p.x,
                             p.y,
                             p.x + button_size.x,
                             p.y + button_size.y + g.FontSize / 2);

    char *cursor = text;
    while (*cursor) {
        if (*cursor == '#' && *(cursor + 1) == '#') {
            break;
        }

        cursor++;
    }
    CREATE_STRING(temp_text, PATH_MAX_SIZE,
                  "%.*s", (int) (cursor - text), text);
    text_centered(temp_text, ImVec2(p.x + padding.x, p.y + image_size.y), ImVec2(button_size.x, button_size.y - image_size.y + padding.y), !hover);
    // draw_list->AddText(g.Font,
    //                    g.FontSize,
    //                    ImVec2(p.x + padding.x, p.y + image_size.y),
    //                    ImGui::GetColorU32(ImGuiCol_Text),
    //                    temp_text,
    //                    NULL,
    //                    button_size.x - padding.x,
    //                    hover ? NULL : &clipping);

    if (hover) {
        draw_list->PopClipRect();
        ImGui::End();
    }

    return result;
}

// @note:
// ---------------padding.y----------------
// padding.x|image|padding.x|text|padding.x
// ---------------padding.y----------------
bool button_with_icon (ImTextureID icon_id, char *text, ImVec2 size = ImVec2(0, 0))
{
    bool result = false;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 padding = style.FramePadding;


    ImVec2 text_size = ImGui::CalcTextSize(text, NULL);
    ImVec2 button_size = ImVec2(size.x == 0 ? text_size.x : size.x,
                                size.y == 0 ? ImGui::GetFrameHeight() - padding.y * 2 :
                                              size.y);
    button_size.x += button_size.y; // @note: icon size


    result = ImGui::InvisibleButton(text,
                                    button_size + padding * 2 + ImVec2(padding.x, 0));

    bool hover = ImGui::IsItemHovered() &&
                 (!ImGui::IsAnyItemActive() || ImGui::IsItemActive());
    bool click = ImGui::IsItemClicked(0);
    ImU32 col = ImGui::GetColorU32((hover && click) ? ImGuiCol_ButtonActive :
                                   hover ? ImGuiCol_ButtonHovered :
                                   ImGuiCol_Button);

    draw_list->AddRectFilled(p,
                             p + button_size + padding * 2 + ImVec2(padding.x, 0),
                             col,
                             g.Style.FrameRounding);

    draw_list->AddImage(icon_id,
                        p + padding,
                        p + padding + ImVec2(button_size.y, button_size.y));

    draw_list->AddText(ImVec2(p.x + button_size.y + padding.x * 2, p.y + padding.y),
                       ImGui::GetColorU32(ImGuiCol_Text),
                       text,
                       NULL);

    return result;
}

// @todo: support unix's forward slash
char *get_last_slash (char *str, bool include_slash = true)
{
    char *last_slash = str;
    int offset = include_slash ? 1 : 0;

    char *temp = str;
    while (*temp) {
        if (*temp == '\\') {
            last_slash = temp + offset;
        }
        temp++;
    }

    return last_slash;
}

void change_dir (App_State *state)
{
    // Back_History *back = &state->back;
}

void add_back (App_State *state)
{
    Back_History *back = &state->tabs[state->current_tab].back;

    assert(back->cursor < back->count);
    back->count = back->cursor + 1;

    int index = (back->start + back->count) % HISTORY_MAX_COUNT;
    strcpy_s(back->history[index], state->tabs[state->current_tab].current_dir);

    if (back->count < HISTORY_MAX_COUNT) {
        back->cursor++;
        back->count++;
    } else {
        back->start = (back->start + 1) % HISTORY_MAX_COUNT;
    }
    // printf("\"%s\" start: %d, cursor: %d, count: %d\n", back->history[index], back->start, back->cursor, back->count);
}

void add_tab (App_State *state, char *start_path = NULL)
{
    if (state->tab_count >= TAB_MAX_COUNT) return;

    int index = 0;
    while (index < TAB_MAX_COUNT && state->tabs[index].id != 0) index++;
    if (index == TAB_MAX_COUNT) assert(!"Couldn't find empty tab to fill!");

    // state->tab_indices[state->tab_count] = index;
    Tab *tab = &state->tabs[index];
    if (start_path == NULL) start_path = state->start_path;

    tab->id = state->tab_gen_id++;
    strcpy_s(tab->current_dir, PATH_MAX_SIZE, start_path);

    tab->back.start = 0;
    tab->back.cursor = 0;
    tab->back.count = 1;
    strcpy_s(tab->back.history[0], start_path);

    tab->selected_count = 0;

    state->tab_count++;
}

void remove_tab (App_State *state, int index)
{
    assert(index > -1 && index < TAB_MAX_COUNT);

    state->tabs[index].id = 0;
    // int temp = index;
    // while (temp < state->tab_count - 1) {
    //     state->tab_indices[temp] = state->tab_indices[temp + 1];
    // }

    state->tab_count--;
}

void go_back (App_State *state)
{
    Back_History *back = &state->tabs[state->current_tab].back;
    if (back->cursor <= 0) return;

    back->cursor--;
    int index = (back->start + back->cursor) % HISTORY_MAX_COUNT;
    strcpy_s(state->tabs[state->current_tab].current_dir, back->history[index]);
}

void go_forward (App_State *state)
{
    Back_History *back = &state->tabs[state->current_tab].back;
    if (back->cursor >= back->count - 1) return;

    back->cursor++;
    int index = (back->start + back->cursor) % HISTORY_MAX_COUNT;
    strcpy_s(state->tabs[state->current_tab].current_dir, back->history[index]);
}

Tab *get_current_tab (App_State *state)
{
    return &state->tabs[state->current_tab];
}

void open_file_or_folder (App_State *state, char *path, bool is_folder)
{
    char *current_dir = get_current_tab(state)->current_dir;
    if (is_folder) {
        snprintf(current_dir, PATH_MAX_SIZE, "%s\\", path);
        add_back(state);
    } else {
        ShellExecute(NULL, NULL, path, NULL, NULL, SW_SHOW);
    }
}

// @todo: "\Handbrake" is illegal!!! must be "D:\Handbrake"
void path_bar (App_State *state)
{
    Tab *current_tab = &state->tabs[state->current_tab];
    char *current_dir = current_tab->current_dir;

    ImVec2 image_button_size = ImVec2(14, 14);

    ImGui::PushStyleColor(ImGuiCol_Button, state->theme.path_button);
    if (ImGui::ImageButton(state->tex_back.id, image_button_size) ||
        (!ImGui::IsAnyItemActive() && // @note: just incase
         ImGui::IsKeyPressed(ImGuiKey_Backspace)))
    {
        go_back(state);
    }
    ImGui::SameLine();
    if (ImGui::ImageButton(state->tex_forward.id, image_button_size)) {
        go_forward(state);
    }
    ImGui::SameLine();
    if (ImGui::ImageButton(state->tex_up.id, image_button_size)) {
        char *first_folder = current_dir;
        char *second_to_last_folder = current_dir;
        char *temp = current_dir;

        int found_backslash = 0;
        while (*temp) {
            if (*temp == '\\') {
                if (*(temp + 1) == '\0') {
                    break;
                } else {
                    second_to_last_folder = temp + 1;
                    found_backslash++;
                }
            }
            temp++;
        }
        if (second_to_last_folder != current_dir) {
            *second_to_last_folder = '\0';
        } else if (found_backslash > 1) {
            *first_folder = '\0';
        }

        add_back(state);
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();

    float size = ImGui::GetContentRegionAvail().x - image_button_size.x * 2 - ImGui::GetStyle().FramePadding.x * 5 - ImGui::CalcTextSize("Search", NULL).x - ImGui::GetStyle().ItemSpacing.x * 2;

    // float start_x = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x;
    if (current_tab->editing) {
        static char prev_path[PATH_MAX_SIZE];

        bool init = false;
        if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
            ImGui::SetKeyboardFocusHere();
            init = true;
            strcpy_s(prev_path, current_tab->current_dir);
        }
        char temp[32];
        snprintf(temp, 32, "##input%d", current_tab->id);
        ImGui::SetNextItemWidth(size + image_button_size.x + ImGui::GetStyle().FramePadding.x * 2);
        if (ImGui::InputText(temp,
                             current_dir,
                             PATH_MAX_SIZE,
                             ImGuiInputTextFlags_EnterReturnsTrue) ||
            ((ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1)) &&
             !ImGui::IsItemHovered()))
        {
            if (!state->path_invalid) {
                remove_multiple_slashes(current_dir);

                // @note: if the path is a valid folder (no trailing word after slash)
                if (get_last_slash(current_dir)[0] == '\0') {
                    add_back(state);
                }
            } else {
                strcpy_s(current_tab->current_dir, prev_path);
            }

            current_tab->editing = false;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            current_tab->editing = false;
        }
        if (init) {
            // @note: clear selection when we first focus
            ImGuiID id = ImGui::GetItemID();
            if (ImGuiInputTextState *state = ImGui::GetInputTextState(id)) {
                state->ClearSelection();
            }
        }
    } else {
        if (ImGui::BeginChild("Path Select",
                          ImVec2(size,
                                 0),
                          false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_HorizontalScrollbar))
        {
            char temp_path[256];
            strcpy_s(temp_path, current_dir);

            bool start = true;
            char *folder, *ctx;
            folder = strtok_s(temp_path, "\\", &ctx);

            int index = 0;
            while (folder) {
                if (start) {
                    ImGui::PushStyleColor(ImGuiCol_Button, state->theme.path_button);
                    start = false;
                }

                char *next_folder = strtok_s(NULL, "\\", &ctx);
                if (!next_folder) {
                    ImGui::PopStyleColor();
                }

                ImGui::PushID(index);
                if (ImGui::Button(folder)) {
                    char *backslash = folder;
                    while (*backslash) backslash++;
                    current_dir[backslash - temp_path + 1] = '\0';
                    // printf("%s\n", current_dir);
                    add_back(state);
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    current_tab->editing = true;
                }
                ImGui::PopID();

                ImGui::SameLine(0, 0);

                folder = next_folder;
                if (folder) {
                    ImGui::Text("|");
                    ImGui::SameLine(0, 0);
                }

                index++;
            }

            static float last_width = ImGui::GetScrollMaxX();
            if (last_width < ImGui::GetScrollMaxX()) {
                ImGui::SetScrollHereX();
            }
            last_width = ImGui::GetScrollMaxX();

            ImGui::EndChild();
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, state->theme.path_button);

        // Edit
        if (ImGui::ImageButton(state->tex_edit.id, image_button_size)) {
            current_tab->editing = true;
        }

        ImGui::SameLine();

        // Search
        if (button_with_icon(state->tex_search.id,
                             "Search",
                             ImVec2(0, image_button_size.y))) {
            state->open_search = true;
        }

        ImGui::PopStyleColor();
    }
}

int group_hash_func (char *str)
{
    int result = 0;
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        result += str[i] * 7 + result * 31;
    }

    return result % GROUP_MAX_COUNT;
}

void set_theme_dark (App_State *state)
{
    Theme *theme = &state->theme;
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(1, 1, 1, 1);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.15, 0.15, 0.15, 1);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.15, 0.15, 0.15, 1);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15, 0.15, 0.15, 1);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25, 0.25, 0.25, 1);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.2, 0.2, 0.2, 1);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2, 0.2, 0.2, 1);
    style.Colors[ImGuiCol_Tab] = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_TabHovered] = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_TabActive] = style.Colors[ImGuiCol_ButtonActive];
    style.Colors[ImGuiCol_FrameBg] = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_FrameBgHovered] = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_FrameBgActive] = style.Colors[ImGuiCol_ButtonActive];

    theme->path_button = ImVec4(0, 0, 0, 0);
    // theme->path_button_hover;
    // theme->path_button_click;

    theme->files_button = ImVec4(0, 0, 0, 0);
    theme->files_button_hover = ImGui::GetStyle().Colors[ImGuiCol_Button];
    // theme->files_button_hover.x *= 0.7;
    // theme->files_button_hover.y *= 0.7;
    // theme->files_button_hover.z *= 0.7;
    // theme->files_button_hover.w = 1;
    theme->files_button_click = ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered];

    theme->files_selected_button = theme->files_button_click;
    theme->files_selected_button_hover = theme->files_button_click;
    // theme->files_selected_button_click;
}

void set_theme_light (App_State *state)
{
    Theme *theme = &state->theme;
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(0, 0, 0, 1);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.75, 0.75, 0.75, 1);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.75, 0.75, 0.75, 1);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.75, 0.75, 0.75, 1);

    style.Colors[ImGuiCol_Button] = ImVec4(0.8, 0.8, 0.8, 1);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.85, 0.85, 0.85, 1);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.85, 0.85, 0.85, 1);
    style.Colors[ImGuiCol_Tab] = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_TabHovered] = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_TabActive] = style.Colors[ImGuiCol_ButtonActive];
    style.Colors[ImGuiCol_FrameBg] = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_FrameBgHovered] = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_FrameBgActive] = style.Colors[ImGuiCol_ButtonActive];

    theme->path_button = ImVec4(0, 0, 0, 0);
    // theme->path_button_hover;
    // theme->path_button_click;

    theme->files_button = ImVec4(0, 0, 0, 0);
    theme->files_button_hover = ImGui::GetStyle().Colors[ImGuiCol_Button];
    theme->files_button_click = ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered];

    theme->files_selected_button = theme->files_button_click;
    theme->files_selected_button_hover = theme->files_button_click;
}

void add_favorite (App_State *state, char *path)
{
    for (int i = 0; i < state->favorite_count; i++) {
        if (strcmp(path, state->favorites[i]) == 0) {
            return;
        }
    }
    strcpy_s(state->favorites[state->favorite_count++], path);
}

void paste(App_State *state)
{
    Tab *tab = get_current_tab(state);
    // @todo: implementation
}

Group *get_group (App_State *state)
{
    Group *result = NULL;

    char *current_dir = state->tabs[state->current_tab].current_dir;
    Group *group = &state->groups[group_hash_func(current_dir)];
    if (strcmp(group->location, current_dir)) {
        result = group;
    } else {
        Group *next = group->next;
        for (; next; next = next->next) {
            if (strcmp(next->location, current_dir)) {
                result = next;
                break;
            }
        }
    }
    return result;
}

void group_button (ImTextureID id, char *text, ImVec2 image_size, ImTextureID tex[4])
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 padding = style.FramePadding;
    ImVec2 text_size = ImGui::CalcTextSize(text, NULL, true, image_size.x - padding.x);
    ImVec2 button_size = ImVec2(image_size.x, image_size.y + (text_size.y < 30 ? text_size.y : 30));

    ImGui::InvisibleButton(text, button_size + padding * 4);

    bool hover = ImGui::IsItemHovered() &&
                 (!ImGui::IsAnyItemActive() || ImGui::IsItemActive());
    bool click = ImGui::IsItemClicked(0);
    ImU32 col = ImGui::GetColorU32(ImGuiCol_PopupBg);

    draw_list->AddRectFilled(p + padding,
                             ImVec2(p.x + button_size.x,
                                    p.y + button_size.y) + padding,
                             col,
                             ImClamp((float)ImMin(padding.x, padding.y),
                                     0.0f,
                                     g.Style.FrameRounding));

    ImVec2 small_image_size = image_size / ImVec2(2, 2);
    float offset = padding.x;
    for (int i = 0; tex[i] && i < 4; i++) {
        float x = ((i % 2) * small_image_size.x);
        float y = (i / 2) * small_image_size.y;
        draw_list->AddImage(tex[i],
                            p + ImVec2(x, y) + padding + ImVec2(offset, 0),
                            p + ImVec2(x, y) + small_image_size - padding + ImVec2(offset, 0));
    }
    ImVec4 clipping = ImVec4(p.x,
                             p.y,
                             p.x + button_size.x,
                             p.y + button_size.y + g.FontSize / 2);

    char *cursor = text;
    while (*cursor) {
        if (*cursor == '#' && *(cursor + 1) == '#') {
            break;
        }

        cursor++;
    }
    CREATE_STRING(temp_text, PATH_MAX_SIZE, "%.*s", (int) (cursor - text), text);
    text_centered(temp_text, ImVec2(p.x + padding.x * 2, p.y + image_size.y), ImVec2(button_size.x, button_size.y - image_size.y + padding.y), !hover);
    // draw_list->AddText(g.Font,
    //                    g.FontSize,
    //                    ImVec2(p.x + padding.x, p.y + image_size.y),
    //                    ImGui::GetColorU32(ImGuiCol_Text),
    //                    temp_text,
    //                    NULL,
    //                    button_size.x - padding.x,
    //                    hover ? NULL : &clipping);
}


#define FILENAME_MAX_COUNT 512
#define FILENAME_MAX_SIZE 128
// @analyze: cache this, and update every so often?
void files_and_folders (App_State *state)
{
    Tab *current_tab = get_current_tab(state);
    char *current_dir = current_tab->current_dir;

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::PushStyleColor(ImGuiCol_Button, state->theme.files_button);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, state->theme.files_button_hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, state->theme.files_button_click);

    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s*", current_dir);

    // @todo: put these in platform specific *_app.cpp
    WIN32_FIND_DATAA file_data;
    HANDLE file_handle = FindFirstFile(path, &file_data);
    if (file_handle == INVALID_HANDLE_VALUE ||
        get_last_slash(path) == path ||
        *path == '\\') {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
        ImGui::TextWrapped("Invalid Path!");
        ImGui::PopStyleColor();
        ImGui::TextWrapped("Might be because the path doesn't exist or Filex doesn't have the permission to access it.");

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        state->path_invalid = true;
        return;
    }
    state->path_invalid = false;
    defer { FindClose(file_handle); };

    char folders[FILENAME_MAX_COUNT][FILENAME_MAX_SIZE];
    uint folder_count = 0;

    char files[FILENAME_MAX_COUNT][FILENAME_MAX_SIZE];
    uint file_count = 0;

    do {
        if (strcmp(file_data.cFileName, ".") == 0 ||
            strcmp(file_data.cFileName, "..") == 0 ||
            strcmp(file_data.cFileName, "$RECYCLE.BIN") == 0 ||
            strcmp(file_data.cFileName, "System Volume Information") == 0) continue;
        if (strlen(file_data.cFileName) > FILENAME_MAX_SIZE) continue;
        if (!state->show_hidden_files && file_data.cFileName[0] == '.') continue;

        if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            assert(folder_count < FILENAME_MAX_COUNT);
            strcpy_s(folders[folder_count++], file_data.cFileName);
        } else {
            assert(file_count < FILENAME_MAX_COUNT);
            strcpy_s(files[file_count++], file_data.cFileName);
        }
    } while (FindNextFile(file_handle, &file_data));

    assert(folder_count + file_count < SELECTED_MAX_COUNT);

    // @note: reset ui when changing folder
    static char prev_path[MAX_PATH];
    if (strcmp(prev_path, current_dir) != 0) {
        ImGui::SetScrollHereY(0);
        current_tab->selected_count = 0;

        strcpy_s(prev_path, current_dir);
    }

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec2 button_size = ImVec2(90, 90);
    float window_visible = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    bool is_selecting = false;

    {
        Group *start_group = &state->groups[group_hash_func(current_dir)];
        Group *group = start_group;

        static uint group_opened_id = 0;
        if (group != NULL && group->id != 0) {
            bool first = false;

            ImVec2 p = ImVec2(0, 0);
            bool open = false;
            for (; group; group = group->next) {
                if (strcmp(group->location, current_dir) != 0) continue;

                ImTextureID tex[4] = {};
                for (int i = 0; i < 4 && i < group->count; i++) {
                    char *text = group->folders[i];

                    char *extension = text;
                    {
                        char *temp = text;
                        while (*temp) {
                            if (*temp == '.') {
                                extension = temp + 1;
                            }
                            temp++;
                        }
                    }
                    if (text == extension) {
                        tex[i] = state->tex_folder.id;
                    } else {
                        tex[i] = get_file_tex(state, extension);
                    }
                }

                ImVec2 temp = ImGui::GetCursorScreenPos();
                CREATE_STRING(label, 64, "%s##%d", group->name, group->id);
                group_button(state->tex_folder.id, label, button_size, tex);
                if (group_opened_id != group->id &&
                    ImGui::IsItemHovered() &&
                    !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
                {
                    if (group_opened_id != group->id) group_opened_id = group->id;
                    open = true;
                }

                if (group_opened_id == group->id) {
                    p = temp - ImGui::GetStyle().WindowPadding;
                }

                float last_button = ImGui::GetItemRectMax().x;
                float next_button = last_button + style.ItemSpacing.x + button_size.x;
                if (next_button < window_visible && group->next != NULL) ImGui::SameLine();
            }
            ImGui::Separator();

            static int skip_frames = 0;
            if (open) {
                ImGui::OpenPopup("Group Hover");
                skip_frames = 0;
            }

            Group *opened_group = NULL;
            if (group_opened_id != 0) {
                Group *group = start_group;
                for (; group; group = group->next) {
                    if (group_opened_id == group->id) {
                        break;
                    }
                }
                opened_group = group;

                ImGui::SetNextWindowFocus();
                ImGui::SetNextWindowBgAlpha(1);
                ImGui::SetNextWindowPos(p.y > ImGui::GetWindowPos().y ? p : ImVec2(p.x, ImGui::GetWindowPos().y));
                ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoSavedSettings;
                float padding = ImGui::GetStyle().FramePadding.y;
                float size = group->count * (button_size.y + 30 + padding);
                if (p.y + size < p.y + ImGui::GetWindowSize().y) {
                    flags |= ImGuiWindowFlags_AlwaysAutoResize;
                } else {
                    ImGui::SetNextWindowSize(ImVec2(0, ImGui::GetWindowSize().y));
                }
                if (ImGui::BeginPopup("Group Hover", flags)) {
                    bool hover = false;
                    for (int i = 0; i < group->count && i < GROUP_FOLDER_MAX_COUNT; i++) {
                        file_button(state->tex_folder.id, group->folders[i], button_size);
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
                            hover = true;

                            if (ImGui::IsMouseClicked(0)) {
                                char temp_path[MAX_PATH];
                                char *last_backslash = get_last_slash(current_dir);
                                int last_backslash_count = last_backslash - current_dir;
                                snprintf(temp_path, PATH_MAX_SIZE,
                                         "%.*s%s",
                                         last_backslash_count, current_dir,
                                         group->folders[i]);

                                open_file_or_folder(state, temp_path, true);
                                group_opened_id = 0;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }

                    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && skip_frames > 8)
                    {
                        group_opened_id = 0;
                        ImGui::CloseCurrentPopup();
                    }
                    skip_frames++;
                    ImGui::EndPopup();
                }

                if (ImGui::IsMouseClicked(1)) {
                    ImGui::OpenPopup("Group Context");
                }
            }

            bool del = false;
            bool open_rename = false;
            if (ImGui::BeginPopup("Group Context")) {
                ImVec2 main_icon_size = ImVec2(20, 20);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                if (ImGui::ImageButton(state->tex_rename.id, main_icon_size)) {
                    open_rename = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();

                if (ImGui::ImageButton(state->tex_delete.id, main_icon_size)) {
                    del = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor();

                ImGui::EndPopup();
            }

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            if (del) ImGui::OpenPopup("Delete Group");

            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Delete Group",
                                       NULL,
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings))
            {
                bool close = false;

                ImGui::Text("Are you sure?\n\n");

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8, 0, 0, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 0, 0, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 1));
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    Group *group = opened_group;

                    Group *prev_group = NULL;
                    Group *next_group = NULL;
                    Group *cursor = start_group;
                    bool done_prev = false;
                    for (; cursor; cursor = cursor->next) {
                        if (cursor == opened_group) {
                            done_prev = true;
                        }
                        if (!done_prev &&
                            strcmp(cursor->location, opened_group->location) == 0)
                        {
                            prev_group = cursor;
                        }

                        if (done_prev &&
                            cursor != opened_group &&
                            strcmp(cursor->location, opened_group->location) == 0)
                        {
                            next_group = cursor;
                            break;
                        }
                    }

                    if (prev_group == NULL) {
                        if (next_group) {
                            assert(strcmp(group->location, next_group->location) == 0);
                            group->id = next_group->id;
                            strcpy_s(group->name, next_group->name);
                            for (uint i = 0; i < next_group->count; i++) {
                                strcpy_s(group->folders[i],
                                         next_group->folders[i]);
                            }
                            group->count = next_group->count;
                            group->next = next_group->next;
                            free(next_group);
                        } else {
                            if (group == start_group) {
                                group->id = 0;
                            } else {
                                free(group);
                            }
                        }
                    } else {
                        if (next_group) {
                            prev_group->next = next_group;
                        } else {
                            prev_group->next = NULL;
                        }
                        free(group);
                    }

                    close = true;
                }
                ImGui::PopStyleColor(3);
                ImGui::SetItemDefaultFocus();

                ImGui::SameLine();

                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    close = true;
                }

                if (close) {
                    group_opened_id = 0;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (open_rename) {
                ImGui::OpenPopup("Rename Group");
            }

            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Rename Group",
                                       NULL,
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings))
            {
                bool close = false;

                Group *group = opened_group;

                static char rename_name[64];
                if (open_rename) {
                    strcpy_s(rename_name, group->name);
                    ImGui::SetKeyboardFocusHere();
                }

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                bool enter = ImGui::InputText("##Rename Group Input",
                                              rename_name,
                                              64,
                                              ImGuiInputTextFlags_EnterReturnsTrue);
                bool escape = ImGui::IsKeyPressed(ImGuiKey_Escape);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.8, 0, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 1, 0, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 1, 0, 1));
                if (ImGui::Button("OK", ImVec2(120, 0)) || enter) {
                    strcpy_s(group->name, rename_name);
                    close = true;
                }
                ImGui::PopStyleColor(3);
                ImGui::SetItemDefaultFocus();

                ImGui::SameLine();

                if (ImGui::Button("Cancel", ImVec2(120, 0)) || escape) {
                    close = true;
                }

                if (close) {
                    group_opened_id = 0;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) {
                group_opened_id = 0;
            }
        }
    }

    for (int i = 0; i < folder_count + file_count; i++) {
        bool is_folder = i < folder_count;
        char label[MAX_PATH];
        int index = is_folder ? i : i - folder_count;

        if (is_folder) {
            snprintf(label, MAX_PATH, ": %s", folders[index]);
        } else {
            snprintf(label, MAX_PATH, "%s", files[index]);
        }

        int found = -1;
        for (int j = 0; j < current_tab->selected_count; j++) {
            if (current_tab->selected[j] != i) continue;

            ImGui::PushStyleColor(ImGuiCol_Button, state->theme.files_selected_button);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  state->theme.files_selected_button_hover);
            found = j;
            break;
        }

        ImVec2 p = ImGui::GetCursorScreenPos();
        if (is_folder) {
            // ImGui::ImageButton(state->tex_folder.id, button_size);
            if (state->list_view) {
                button_with_icon(state->tex_folder.id, folders[index], ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0));
            } else {
                file_button(state->tex_folder.id, folders[index], button_size);
            }
        } else {
            char *text = files[index];

            char *extension = text;
            {
                char *temp = text;
                while (*temp) {
                    if (*temp == '.') {
                        extension = temp + 1;
                    }
                    temp++;
                }
            }
            ImTextureID id = get_file_tex(state, extension);

            if (state->list_view) {
                button_with_icon(id, text, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0));
            } else {
                file_button(id, text, button_size);
            }
        }

        if (!state->list_view) {
            float last_button = ImGui::GetItemRectMax().x;
            float next_button = last_button + style.ItemSpacing.x + button_size.x;
            // float next_button = p.x + button_size.x + style.ItemSpacing.x + button_size.x;
            if (next_button < window_visible) ImGui::SameLine();
        }

        // @note: since ImGui::Button is delayed, we put it here
        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseClicked(0))
        {
            if (ImGui::GetIO().KeyShift) {
                if (found > -1) {
                    current_tab->selected[found] = current_tab->selected[current_tab->selected_count-- - 1];
                } else {
                    current_tab->selected[current_tab->selected_count++] = i;
                }
            } else {
                current_tab->selected_count = 0;
                current_tab->selected[current_tab->selected_count++] = i;
            }

            is_selecting = true;
        }

        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(0) &&
            !ImGui::GetIO().KeyShift)
        {
            // @note: since searching is done in the path bar, we have to
            // get the current_dir without the search
            char temp_path[MAX_PATH];
            char *last_backslash = get_last_slash(current_dir);
            int last_backslash_count = last_backslash - current_dir;
            snprintf(temp_path, PATH_MAX_SIZE,
                     "%.*s%s",
                     last_backslash_count, current_dir,
                     is_folder ? folders[index] : files[index]);

            open_file_or_folder(state, temp_path, is_folder);
        }
        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseClicked(1))
        {
            // current_tab->selected_count = 0;
            // bool found = false;
            // for (int j = 0; j < current_tab->selected_count; j++) {
            //     if (i == current_tab->selected[j]) {
            //         found = true;
            //         break;
            //     }
            // }
            if (found == -1) {
                current_tab->selected_count = 0;
                current_tab->selected[current_tab->selected_count++] = i;
            }
            ImGui::OpenPopup("File Context");
        }

        if (found > -1) {
            ImGui::PopStyleColor(2);
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    // @note: clicked outside any files/folders
    if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId) &&
        ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(1))
    {
        current_tab->selected_count = 0;
        ImGui::OpenPopup("File Context");
    }

    bool del = false;
    bool open_rename = false;
    bool open_group_name = false;

    ImVec2 main_icon_size = ImVec2(20, 20);
    ImGui::SetNextWindowSize(ImVec2(190, 0));
    if (ImGui::BeginPopup("File Context")) {
        if (current_tab->selected_count > 0) {
            bool close = false;

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            if (ImGui::ImageButton(state->tex_copy.id, main_icon_size)) {
                // @todo
                if (state->copy) {
                    free(state->copy);
                }
                state->copy = (char (*)[PATH_MAX_SIZE])malloc(current_tab->selected_count *
                                               sizeof(*state->copy));
                state->copy_count = current_tab->selected_count;
                state->cut = false;

                for (int i = 0; i < current_tab->selected_count; i++) {
                    int index = current_tab->selected[i];
                    bool is_folder = index < folder_count;

                    char *last_backslash = get_last_slash(current_dir);
                    int last_backslash_count = last_backslash - current_dir;
                    snprintf(state->copy[i], PATH_MAX_SIZE,
                             "%.*s%s",
                             last_backslash_count, current_dir,
                             is_folder ? folders[index] :
                                         files[index - folder_count]);
                }
                // for (int i = 0; i < state->copy_count; i++) {
                //     printf("%s\n", state->copy[i]);
                // }
                close = true;
            }
            ImGui::SameLine();

            if (ImGui::ImageButton(state->tex_cut.id, main_icon_size)) {
                // @todo
                state->cut = false;
                close = true;
            }
            ImGui::SameLine();

            if (ImGui::ImageButton(state->tex_paste.id, main_icon_size)) {
                // @todo
                close = true;
            }
            ImGui::SameLine();

            if (ImGui::ImageButton(state->tex_rename.id, main_icon_size)) {
                open_rename = true;
                close = true;
            }
            ImGui::SameLine();

            if (ImGui::ImageButton(state->tex_delete.id, main_icon_size)) {
                del = true;
                close = true;
            }
            ImGui::PopStyleColor();

            ImGui::Separator();

            int index = current_tab->selected[current_tab->selected_count - 1];
            bool is_folder = index < folder_count;

            float full_width = ImGui::GetContentRegionAvail().x;
            ImVec2 button_size = ImVec2(full_width - 25, 0);
            if (button_with_icon(is_folder ? state->tex_folder.id :
                                             state->tex_file.id,
                                 "Open", button_size)) {
                char temp_path[MAX_PATH];
                char *last_backslash = get_last_slash(current_dir);
                int last_backslash_count = last_backslash - current_dir;

                snprintf(temp_path, PATH_MAX_SIZE,
                         "%.*s%s",
                         last_backslash_count, current_dir,
                         is_folder ? folders[index] :
                                     files[index - folder_count]);
                open_file_or_folder(state, temp_path, is_folder);
                close = true;
            }

            if (is_folder) {
                if (button_with_icon(state->tex_folder.id, "Open in New Tab", button_size)) {
                    char temp_path[MAX_PATH];
                    char *last_backslash = get_last_slash(current_dir);
                    int last_backslash_count = last_backslash - current_dir;

                    snprintf(temp_path, PATH_MAX_SIZE,
                             "%.*s%s\\",
                             last_backslash_count, current_dir,
                             folders[index]);
                    add_tab(state, temp_path);
                    close = true;
                }
            }

            ImGui::Separator();

            if (button_with_icon(state->tex_folder.id, "New Group", button_size)) {
                open_group_name = true;
                close = true;
            }


            // @todo: for files, we disable it for now
            if (is_folder &&
                button_with_icon(state->tex_favorite.id, "Add to Favorites", button_size)) {
                int index = current_tab->selected[current_tab->selected_count - 1];
                char temp_path[MAX_PATH];
                char *last_backslash = get_last_slash(current_dir);
                int last_backslash_count = last_backslash - current_dir;

                snprintf(temp_path, PATH_MAX_SIZE,
                         "%.*s%s",
                         last_backslash_count, current_dir,
                         is_folder ? folders[index] :
                                     files[index - folder_count]);
                add_favorite(state, temp_path);

                close = true;
            }

            ImGui::Separator();

            if (button_with_icon(state->tex_info.id,
                                 "Properties",
                                 button_size))
            {
                close = true;
            }

            if (close) ImGui::CloseCurrentPopup();
        } else {
            bool close = false;

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::BeginDisabled(true);
            ImGui::ImageButton(state->tex_copy.id, main_icon_size);
            ImGui::SameLine();
            ImGui::ImageButton(state->tex_cut.id, main_icon_size);
            ImGui::SameLine();
            if (state->copy_count > 0) ImGui::EndDisabled();

            if (ImGui::ImageButton(state->tex_paste.id, main_icon_size)) {
                // @note: copy_count > 0
                paste(state);
                close = true;
            }
            ImGui::SameLine();

            if (state->copy_count > 0) ImGui::BeginDisabled(true);
            ImGui::ImageButton(state->tex_rename.id, main_icon_size);
            ImGui::SameLine();
            ImGui::ImageButton(state->tex_delete.id, main_icon_size);
            ImGui::EndDisabled();
            ImGui::PopStyleColor();

            ImGui::Separator();

            ImGuiStyle &style = ImGui::GetStyle();
            float width = ImGui::GetContentRegionAvail().x - 25;
            if (button_with_icon(state->tex_file.id, "New File", ImVec2(width, 0))) {
                close = true;
            }

            if (button_with_icon(state->tex_folder.id, "New Folder", ImVec2(width, 0))) {
                close = true;
            }

            ImGui::Separator();

            char *text = "Show in list view";
            if (state->list_view) text = "Show in icon view";
            if (button_with_icon(state->tex_folder.id,
                                 text,
                                 ImVec2(width, 0)))
            {
                state->list_view = !state->list_view;
                close = true;
            }

            ImGui::Separator();

            if (close) ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (del) ImGui::OpenPopup("Delete");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Delete",
                               NULL,
                               ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoMove |
                               ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::Text("Are you sure?\n\n");
        static bool dont_ask_me_next_time = false;
        ImGui::Checkbox("Don't ask for this session", &dont_ask_me_next_time);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8, 0, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 0, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 1));
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            int index = current_tab->selected[current_tab->selected_count - 1];
            char temp_path[PATH_MAX_SIZE];
            char *last_backslash = get_last_slash(current_dir);
            int last_backslash_count = last_backslash - current_dir;
            char *name = index < folder_count ? folders[index] : files[index - folder_count];
            snprintf(temp_path, PATH_MAX_SIZE, "%.*s%s", last_backslash_count, current_dir, name);

            wchar_t filename[PATH_MAX_SIZE];
            MultiByteToWideChar(CP_UTF8, 0, temp_path, PATH_MAX_SIZE, filename, PATH_MAX_SIZE);

            IShellItem *shell_item = NULL;
            if (SHCreateItemFromParsingName(filename,
                                            NULL,
                                            IID_PPV_ARGS(&shell_item)) == S_OK)
            {
                IFileOperation *operation;
                if (CoCreateInstance(CLSID_FileOperation,
                                     NULL,
                                     CLSCTX_ALL,
                                     IID_PPV_ARGS(&operation)) == S_OK)
                {
                    operation->SetOperationFlags(FOF_ALLOWUNDO | FOF_WANTNUKEWARNING);
                    operation->DeleteItem(shell_item, NULL);
                    operation->PerformOperations();
                }
                shell_item->Release();
            }

            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (open_rename) {
        ImGui::OpenPopup("Rename");
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Rename",
                               NULL,
                               ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoMove |
                               ImGuiWindowFlags_NoSavedSettings))
    {
        static char rename_name[64];
        if (open_rename) {
            int index = current_tab->selected[current_tab->selected_count - 1];
            strcpy_s(rename_name, index < folder_count ? folders[index] : files[index - folder_count]);
            ImGui::SetKeyboardFocusHere();
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        bool enter = ImGui::InputText("##Rename Input",
                                      rename_name,
                                      64,
                                      ImGuiInputTextFlags_EnterReturnsTrue);
        bool escape = ImGui::IsKeyPressed(ImGuiKey_Escape);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.8, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 1, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 1, 0, 1));
        if (ImGui::Button("OK", ImVec2(120, 0)) || enter) {
            int index = current_tab->selected[current_tab->selected_count - 1];
            char temp_path[PATH_MAX_SIZE];
            char *last_backslash = get_last_slash(current_dir);
            int last_backslash_count = last_backslash - current_dir;
            char *name = index < folder_count ? folders[index] : files[index - folder_count];

            snprintf(temp_path, PATH_MAX_SIZE, "%.*s%s", last_backslash_count, current_dir, name);
            wchar_t old_name[PATH_MAX_SIZE];
            MultiByteToWideChar(CP_UTF8, 0, temp_path, PATH_MAX_SIZE, old_name, PATH_MAX_SIZE);

            // @todo: PathStripPathW probably just gets the filename,
            // so we don't need to concat current_dir
            snprintf(temp_path, PATH_MAX_SIZE, "%.*s%s", last_backslash_count, current_dir, rename_name);
            wchar_t new_name[PATH_MAX_SIZE];
            MultiByteToWideChar(CP_UTF8, 0, temp_path, PATH_MAX_SIZE, new_name, PATH_MAX_SIZE);
            PathStripPathW(new_name);

            IShellItem *shell_item = NULL;
            if (SHCreateItemFromParsingName(old_name,
                                            NULL,
                                            IID_PPV_ARGS(&shell_item)) == S_OK)
            {
                IFileOperation *operation;
                if (CoCreateInstance(CLSID_FileOperation,
                                     NULL,
                                     CLSCTX_ALL,
                                     IID_PPV_ARGS(&operation)) == S_OK)
                {
                    operation->SetOperationFlags(FOF_ALLOWUNDO | FOF_SILENT);
                    operation->RenameItem(shell_item, new_name, NULL);
                    operation->PerformOperations();
                }
                shell_item->Release();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)) || escape) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (open_group_name) ImGui::OpenPopup("Group Name");

    if (ImGui::BeginPopupModal("Group Name",
                               NULL,
                               ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoMove |
                               ImGuiWindowFlags_NoSavedSettings))
    {
        static char group_name[64];
        if (open_group_name) {
            strcpy_s(group_name, "New Group");
            ImGui::SetKeyboardFocusHere();
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        bool enter = ImGui::InputText("##Group Name Input",
                                      group_name,
                                      64,
                                      ImGuiInputTextFlags_EnterReturnsTrue);
        bool escape = ImGui::IsKeyPressed(ImGuiKey_Escape);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.8, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 1, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 1, 0, 1));
        if (ImGui::Button("OK", ImVec2(120, 0)) || enter) {
            Group *group = &state->groups[group_hash_func(current_dir)];
            if (group->id != 0) {
                Group *next = group;
                for (; next->next; next = next->next) {}
                assert(next->next == NULL);
                next->next = (Group *) malloc(sizeof(Group));
                group = next->next;
            }
            *group = {};
            group->id = state->group_gen_id++;
            strcpy_s(group->name, group_name);
            strcpy_s(group->location, current_dir);

            for (int i = 0; i < current_tab->selected_count && i < GROUP_FOLDER_MAX_COUNT; i++) {
                int index = current_tab->selected[i];
                bool is_folder = index < folder_count;
                snprintf(group->folders[i], 64, is_folder ? folders[index] : files[index - folder_count]);
            }
            group->count = current_tab->selected_count;
            current_tab->selected_count = 0;

            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)) || escape) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }


    // @note: klik di luar buttons
    if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId) &&
        ImGui::IsMouseClicked(0) &&
        !is_selecting)
    {
        current_tab->selected_count = 0;
    }
}

void navigation (App_State *state)
{
    char *current_dir = state->tabs[state->current_tab].current_dir;

    static bool show_favorites = true;
    static bool show_this_pc = true;

    // Favorites
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(show_favorites ? "v Favorites" : "> Favorites")) show_favorites = !show_favorites;
    ImGui::PopStyleColor();
    if (show_favorites) {
        for (uint i = 0; i < state->favorite_count; i++) {
            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();

            char name[MAX_PATH];
            char *last_backslash = get_last_slash(current_dir);
            int last_backslash_count = last_backslash - current_dir;
            snprintf(name, MAX_PATH, "%.*s##d", last_backslash_count, state->favorites[i], i);
            if (button_with_icon(state->tex_folder.id, state->favorites[i])) {
                open_file_or_folder(state, state->favorites[i], true);
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
                state->selected_fav_index = i;
                ImGui::OpenPopup("Favorites");
            }
        }

        if (ImGui::BeginPopup("Favorites")) {
            if (ImGui::MenuItem("Open")) {
                open_file_or_folder(state, state->favorites[state->selected_fav_index], true);
            }
            if (ImGui::MenuItem("Remove from Favorites")) {
                for (int i = state->selected_fav_index;
                     i < state->favorite_count;
                     i++) {
                    strcpy_s(state->favorites[i], state->favorites[i + 1]);
                }
                state->favorite_count--;
            }

            ImGui::EndPopup();
        }
    }
    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(show_this_pc ? "v This PC" : "> This PC")) show_this_pc = !show_this_pc;
    ImGui::PopStyleColor();
    if (show_this_pc) {
        char *folder_name[] = {
            "Documents",
            "Downloads",
            "Music",
            "Pictures",
            "Videos",
        };

        ImTextureID folder_tex[] = {
            state->tex_file_word.id,
            state->tex_down.id,
            state->tex_file_mp3.id,
            state->tex_file_image.id,
            state->tex_file_mp4.id,
        };
        // @note: https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid
        KNOWNFOLDERID folder_id[] = {
            FOLDERID_Documents,
            FOLDERID_Downloads,
            FOLDERID_Music,
            FOLDERID_Pictures,
            FOLDERID_Videos,
        };
        assert(IM_ARRAYSIZE(folder_name) == IM_ARRAYSIZE(folder_tex) &&
               IM_ARRAYSIZE(folder_name) == IM_ARRAYSIZE(folder_id));

        for (int i = 0; i < IM_ARRAYSIZE(folder_name); i++) {
            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();
            if (button_with_icon(folder_tex[i], folder_name[i])) {
                wchar_t *wpath;
                char path[MAX_PATH];
                if (SHGetKnownFolderPath(folder_id[i], 0, NULL, &wpath) == S_OK) {
                    int wlen = lstrlenW(wpath);
                    WideCharToMultiByte(CP_ACP, 0, wpath, -1, path, MAX_PATH, NULL, NULL);
                    snprintf(current_dir, PATH_MAX_SIZE, "%s\\", path);
                    add_back(state);
                }
                CoTaskMemFree(wpath);
            }
        }

        ImGui::Dummy(ImVec2(0, ImGui::GetStyle().FramePadding.y * 2));

        // @note: bit ke-0 untuk drive A, ke-1 untuk B, dst.
        // @todo: is GetLogicalDrives expensive??
        u32 drives = GetLogicalDrives();
        for (int i = 0; i < sizeof(drives) * 8; i++) {
            if (!(drives & (1 << i))) continue;

            char drive[64];

            char drive_name[MAX_PATH + 1];
            char drive_letter[8];
            snprintf(drive_letter, 8, "%c:\\", 65 + i);
            if (GetVolumeInformation(drive_letter, drive_name, MAX_PATH + 1, NULL, NULL, 0, NULL, 0)) {
                snprintf(drive, 64, "[%c] %s", 65 + i, drive_name);
            } else {
                snprintf(drive, 64, "[%c]", 65 + i);
            }

            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();
            if (ImGui::Button(drive)) {
                strcpy_s(current_dir, PATH_MAX_SIZE, drive_letter);
                add_back(state);
            }
        }
    }
    ImGui::Separator();
}

size_t read_file (char *&buffer, char *path)
{
    FILE *file;
    fopen_s(&file, path, "r");

    if (!file) {
        // fprintf(stderr, "Failed to open file: %s\n", path);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // @note: file_size will always be bigger than read_size
    // because fopen 'r' replaces '\r\n' with '\n'
    buffer = (char *) malloc(file_size + 1);
    size_t read_size = fread(buffer, sizeof(char), file_size, file);
    buffer[read_size++] = '\0';

    fclose(file);

    return read_size;
}

char *our_strcat (char *src, char *str, char *start = NULL)
{
    char *cursor = start ? start : src;
    while (*cursor) cursor++;
    while (*str) {
        *(cursor++) = *(str++);
    }

    return cursor;
}

// @todo: quick and dirty parser
void load_settings (App_State *state)
{
    char *settings_file;
    if (read_file(settings_file, SETTINGS_FILENAME) == 0) {
        state->tex_wallpaper = get_app_texture(state, "wallpaper0.png");
        snprintf(state->wallpaper, PATH_MAX_SIZE, "%sdata\\wallpaper0.png", state->exe_path);

        // fprintf(stderr, "Couldn't find \"%s\"\n", SETTINGS_FILENAME);
        return;
    }
    defer { free(settings_file); };

    // @note: too lazy to make an enum
    const int NONE = 0;
    const int FAVORITES = 1;
    const int SETTINGS = 2;
    int context = NONE;

    char wallpaper[PATH_MAX_SIZE];
    strcpy_s(wallpaper, state->wallpaper);

    char *line_str, *line_ctx;
    line_str = strtok_s(settings_file, "\n", &line_ctx);


    int line_number = 0;
    while (line_str) {
        line_number++;

        // printf("%s\n", line_str);
        bool change_context = false;
        switch (context) {
            case FAVORITES: {
                if (line_str[0] != '@') {
                    add_favorite(state, line_str);
                } else {
                    change_context = true;
                }
            } break;

            case SETTINGS: {
                if (line_str[0] != '@') {
                    char *word_str, *word_ctx;
                    word_str = strtok_s(line_str, " ", &word_ctx);
                    if (strcmp(word_str, "start_path") == 0) {
                        word_str = strtok_s(NULL, "\n", &word_ctx);
                        strcpy_s(state->start_path, word_str);
                    } else if (strcmp(word_str, "wallpaper") == 0) {
                        word_str = strtok_s(NULL, "\n", &word_ctx);
                        strcpy_s(wallpaper, word_str);
                    } else if (strcmp(word_str, "light_mode") == 0) {
                        word_str = strtok_s(NULL, "\n", &word_ctx);
                        if (*word_str == '1') {
                            state->light_mode = true;
                        } else {
                            state->light_mode = false;
                        }
                    }
                } else {
                    change_context = true;
                }
            } break;

            case NONE:
            default: {
                change_context = true;
            } break;
        }

        if (change_context) {
            if (line_str[0] == '@') {
                if (strcmp(line_str, "@ fav") == 0) {
                    context = FAVORITES;
                } else if (strcmp(line_str, "@ set") == 0) {
                    context = SETTINGS;
                }
            }
        }

        line_str = strtok_s(NULL, "\n", &line_ctx);
    }

    Texture tex_wallpaper = {};
    char *cursor = wallpaper;
    char *extension = wallpaper;
    while (*cursor) {
        if (*cursor == '.') {
            extension = cursor + 1;
        }
        cursor++;
    }
    tex_wallpaper = get_texture(wallpaper);
    if (tex_wallpaper.id == 0) {
        state->tex_wallpaper = get_app_texture(state, "wallpaper0.png");
        snprintf(state->wallpaper, PATH_MAX_SIZE, "%sdata\\wallpaper0.png", state->exe_path);
    } else {
        state->tex_wallpaper = tex_wallpaper;
        strcpy_s(state->wallpaper, wallpaper);
    }
}

#if 0
struct Writer {
    char *buffer;
    size_t buffer_size;

    char *cursor;
    int count;
};

void write (Writer *writer, char *format, ...)
{
    va_list va;
    va_start(va, format);

    // while (true) {
    //     int written = vsnprintf(ptr, SETTINGS_MAX_SIZE - count, format, va);
    //     if (count + written < SETTINGS_MAX_SIZE) break;
    // }
    int written = vsnprintf(writer->cursor, SETTINGS_MAX_SIZE - writer->count, format, va);
    if (writer->count + written + 1 >= SETTINGS_MAX_SIZE) {
        assert(!"exceeded SETTINGS_MAX_SIZE");
    }

    writer->count += written;
    writer->cursor += written;
    va_end(va);
}
#endif

#define SETTINGS_MAX_SIZE 8192
void save_settings (App_State *state)
{
    char *filename = SETTINGS_FILENAME;
    char temp_path[PATH_MAX_SIZE];
    snprintf(temp_path, PATH_MAX_SIZE, "%s\\%s", state->exe_path, filename);

    // Writer writer = {};

    char buffer[SETTINGS_MAX_SIZE];
    char *ptr = buffer;
    int count = 0;

    int written = snprintf(ptr, SETTINGS_MAX_SIZE - count, "!!! TOUCH WITH CARE !!!\n");
    count += written;
    ptr += written;

    written = snprintf(ptr, SETTINGS_MAX_SIZE - count, "@ set\n");
    count += written;
    ptr += written;

    written = snprintf(ptr,
                       SETTINGS_MAX_SIZE - count,
                       "start_path %s\n",
                       state->start_path);
    count += written;
    ptr += written;

    written = snprintf(ptr,
                       SETTINGS_MAX_SIZE - count,
                       "wallpaper %s\n",
                       state->wallpaper);
    count += written;
    ptr += written;

    written = snprintf(ptr,
                       SETTINGS_MAX_SIZE - count,
                       "light_mode %s\n",
                       state->light_mode ? "1" : "0");
    count += written;
    ptr += written;

    written = snprintf(ptr, SETTINGS_MAX_SIZE - count, "@ fav\n");
    count += written;
    ptr += written;

    for (uint i = 0; i < state->favorite_count; i++) {
        int written = snprintf(ptr, SETTINGS_MAX_SIZE - count,
                               "%s\n", state->favorites[i]);
        count += written;
        ptr += written;
    }

    FILE *file;
    errno_t err = fopen_s(&file, temp_path, "w");
    if (err != 0) {
        fprintf(stderr, "failed to save \"%s\"\n", SETTINGS_FILENAME);
        return;
    }
    fwrite(buffer, count, 1, file);
    fclose(file);
}

// uint WINAPI thread_proc (void *parameter)
// {
//     return 0;
// }

void app (SDL_Window *window)
{
    // @todo: create new thread for search
    // uint thread_id = 0;
    // HANDLE thread_handle = CreateThread(NULL, 0, thread_proc, 0, &thread_id);

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigInputTextCursorBlink = false;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0;
    style.FrameRounding = style.GrabRounding = 12;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    style.PopupRounding = 6;

    App_State state;

    // @note: executable's folder path
    GetModuleFileName(NULL, state.exe_path, PATH_MAX_SIZE);
    *(get_last_slash(state.exe_path) - 1) = '\0';
    printf("%s\n", state.exe_path);

    load_settings(&state);

    if (state.light_mode) {
        set_theme_light(&state);
    } else {
        set_theme_dark(&state);
    }

    state.tex_folder     = get_app_texture(&state, "folders.png");
    state.tex_file       = get_app_texture(&state, "file.png");
    state.tex_edit       = get_app_texture(&state, "edit.png");
    state.tex_back       = get_app_texture(&state, "back.png");
    state.tex_forward    = get_app_texture(&state, "forward.png");
    state.tex_up         = get_app_texture(&state, "up.png");
    state.tex_down       = get_app_texture(&state, "down.png");
    state.tex_search     = get_app_texture(&state, "search.png");
    state.tex_info       = get_app_texture(&state, "info.png");

    state.tex_file_exe   = get_app_texture(&state, "exe.png");
    state.tex_file_txt   = get_app_texture(&state, "txt.png");
    state.tex_file_mp3   = get_app_texture(&state, "mp3.png");
    state.tex_file_mp4   = get_app_texture(&state, "mp4.png");
    state.tex_file_image = get_app_texture(&state, "image.png");
    state.tex_file_zip   = get_app_texture(&state, "zip.png");
    state.tex_file_word  = get_app_texture(&state, "word.png");

    state.tex_copy   = get_app_texture(&state, "copy.png");
    state.tex_paste  = get_app_texture(&state, "paste.png");
    state.tex_rename = get_app_texture(&state, "rename.png");
    state.tex_cut    = get_app_texture(&state, "cut.png");
    state.tex_delete = get_app_texture(&state, "delete.png");
    state.tex_favorite = get_app_texture(&state, "favorite.png");

    bool run = true;
    while (run) {
        // -------- UPDATE
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) run = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window)) run = false;
        }

        state.current_tab = -1;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowBgAlpha(1);

        ImGuiWindowFlags main_window_flags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoFocusOnAppearing;

        if (ImGui::Begin("Main", NULL, main_window_flags)) {
            // ImGui::DockSpace(ImGui::GetID("Main Dock"), ImVec2(0, 0), ImGuiDockNodeFlags_KeepAliveOnly);
            // Background
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                ImVec2 p = ImGui::GetWindowPos();
                ImVec2 s = ImGui::GetWindowSize();
                if (state.tex_wallpaper.height * (s.x / state.tex_wallpaper.width) < s.y) {
                    s.x = state.tex_wallpaper.width * (s.y / state.tex_wallpaper.height);
                } else {
                    s.y = state.tex_wallpaper.height * (s.x / state.tex_wallpaper.width);
                }
                draw_list->PushClipRectFullScreen();
                draw_list->AddImage(state.tex_wallpaper.id,
                                    p,
                                    p + s);
                draw_list->PopClipRect();
            }

            // Menu
            bool open_faq = false;
            bool open_doc = false;
            bool open_about = false;
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New Tab", "CTRL+T")) {
                        add_tab(&state);
                    }
                    ImGui::Separator();

                    if (ImGui::MenuItem("Exit", "CTRL+Q")) {
                        run = false;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Undo", "CTRL+Z", false, false)) {}
                    if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}
                    ImGui::Separator();
                    if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                    if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                    if (ImGui::MenuItem("Paste", "CTRL+V")) {}
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Settings")) {
                    ImGui::MenuItem("View", NULL, false, false);

                    if (ImGui::MenuItem("Change Wallpaper")) {
                        char filename[MAX_PATH];
                        if (open_file_dialog(filename)) {
                            char *cursor = filename;
                            char *extension = filename;
                            while (*cursor) {
                                if (*cursor == '.') {
                                    extension = cursor + 1;
                                }
                                cursor++;
                            }

                            Texture tex_wallpaper = {};
                            tex_wallpaper = get_texture(filename);
                            if (tex_wallpaper.id != 0) {
                                glDeleteTextures(1,
                                                 &((GLuint)(intptr_t) state.tex_wallpaper.id));
                                state.tex_wallpaper = tex_wallpaper;
                                strcpy_s(state.wallpaper, filename);
                            }
                        }
                    }

                    if (ImGui::MenuItem("Light Mode", "", &state.light_mode)) {
                        if (state.light_mode) {
                            set_theme_light(&state);
                        } else {
                            set_theme_dark(&state);
                        }
                    }

                    ImGui::MenuItem("Show navigation panel",
                                    "",
                                    &state.show_navigation_panel);
                    // ImGui::MenuItem("Files and Folders", NULL, false, false);
                    ImGui::MenuItem("Show in list view",
                                    "",
                                    &state.list_view);
                    ImGui::MenuItem("Show hidden files",
                                    "",
                                    &state.show_hidden_files);
                    ImGui::Separator();

                    ImGui::MenuItem("Tabs", NULL, false, false);
                    ImGui::MenuItem("Show tab list popup button",
                                    "",
                                    &state.show_tab_list_popup_button);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help")) {
                    if (ImGui::MenuItem("FAQ", "")) {
                        open_faq = true;
                    }
                    if (ImGui::MenuItem("Documentation", "")) {
                        open_doc = true;
                    }
                    if (ImGui::MenuItem("About Filex", "")) {
                        open_about = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            if (open_faq) ImGui::OpenPopup("FAQ");
            if (open_doc) ImGui::OpenPopup("Documentation");
            if (open_about) ImGui::OpenPopup("About Filex");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoResize |
                                     // ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoSavedSettings;
            ImGui::SetNextWindowSize(ImVec2(500, 360));
            if (ImGui::BeginPopupModal("FAQ", NULL, flags)) {
                ImGui::BeginChild("FAQ Text", ImVec2(400, 300));

                ImGui::TextWrapped("Q: Bagaimana cara mengedit path?");
                ImGui::TextWrapped("A: Untuk mengedit path, bisa menekan tombol edit di samping tombol search, atau klik kiri 2x pada path");
                ImGui::Dummy(ImVec2(0, ImGui::GetStyle().FramePadding.y * 2));

                ImGui::TextWrapped("Q: Bagaimana cara menambahkan folder ke favorit?");
                ImGui::TextWrapped("A: Pilihlah folder, lalu klik kanan pada mouse, dan pilih \"Add to Favorites\"");
                ImGui::Dummy(ImVec2(0, ImGui::GetStyle().FramePadding.y * 2));

                ImGui::TextWrapped("Q: Bagaimana cara membuat grup?");
                ImGui::TextWrapped("A: Pilihlah folder-folder yang ingin dijadikan grup, lalu klik kanan pada mouse, pilih \"New Group\", masukkan nama grup, dan klik \"OK\"");
                ImGui::Dummy(ImVec2(0, ImGui::GetStyle().FramePadding.y * 2));

                ImGui::TextWrapped("Q: Bagaimana cara mengganti wallpaper?");
                ImGui::TextWrapped("A: Klik menu \"Settings\", klik \"Change Wallpaper\", pilih gambar yang dinginkan, lalu klik \"Open\"");
                ImGui::Dummy(ImVec2(0, ImGui::GetStyle().FramePadding.y * 2));

                ImGui::EndChild();

                if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Documentation", NULL, flags)) {
                ImGui::TextWrapped("hi");
                ImGui::Dummy(ImVec2(0, ImGui::GetStyle().FramePadding.y * 2));
                if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("About Filex", NULL, flags)) {
                ImGui::TextWrapped("Filex is made by:");
                ImGui::TextWrapped("Moh. Rizki Syaifudin           (4121002)");
                ImGui::TextWrapped("Reginal Dariel Chandra Pandega (4121047)");
                ImGui::TextWrapped("Achmad Fadhillah Azzkri        (4121048)");
                ImGui::TextWrapped("Syafiq Ama Syauqillah          (4121054)");
                ImGui::Dummy(ImVec2(0, ImGui::GetStyle().FramePadding.y * 2));
                if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            // Menu Shortcuts
            if (ImGui::IsKeyPressed(ImGuiKey_T) && io.KeyCtrl) {
                add_tab(&state);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Q) && io.KeyCtrl) {
                run = false;
            }

            if (state.tab_count == 0) {
                add_tab(&state);
            }

            // Tabs
            // @todo: i prefer FittingPolicyScroll, but for some reason you can't
            // scroll with mouse so it's useless. check if it gets fixed.
            // add an option later?
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs |
                                             ImGuiTabBarFlags_Reorderable |
                                             ImGuiTabBarFlags_FittingPolicyResizeDown;
                                             // ImGuiTabBarFlags_FittingPolicyScroll |
                                             // ImGuiTabBarFlags_NoTabListScrollingButtons;
            if (state.show_tab_list_popup_button) tab_bar_flags |= ImGuiTabBarFlags_TabListPopupButton;

            if (ImGui::BeginTabBar("MainTabBar", tab_bar_flags)) {
                if (ImGui::TabItemButton("+",
                                         ImGuiTabItemFlags_Trailing |
                                         ImGuiTabItemFlags_NoTooltip))
                {
                    add_tab(&state);
                }

                for (int i = 0; i < TAB_MAX_COUNT; i++) {
                    if (state.tabs[i].id == 0) continue;

                    state.current_tab = i;

                    bool open = true;

                    char tab_name[32];

                    char temp_path[PATH_MAX_SIZE];
                    strcpy_s(temp_path, state.tabs[state.current_tab].current_dir);
                    remove_multiple_slashes(temp_path);
                    char *last_folder = temp_path;
                    char *temp = temp_path;
                    while (*temp) {
                        if (*temp == '\\') {
                            if (*(temp + 1) == '\0') {
                                break;
                            } else {
                                last_folder = temp + 1;
                            }
                        }
                        temp++;
                    }
                    char temp_name[32];
                    snprintf(temp_name, 16, "%.*s", (int) (temp - last_folder), last_folder);
                    snprintf(tab_name, 32, "%s###tab%d", temp_name, state.tabs[i].id);

                    if (ImGui::BeginTabItem(tab_name, &open)) {
                        // Path
                        ImGui::BeginChild("Path Bar",
                                          ImVec2(0, 20));
                        path_bar(&state);
                        ImGui::EndChild();
                        ImGui::Separator();

                        // Navigation
                        if (state.show_navigation_panel) {
                            // @todo: turn this into normal window
                            ImGui::BeginChild("Navigation##Global",
                                              ImVec2(ImGui::GetContentRegionAvail().x * 0.2f,
                                                     ImGui::GetContentRegionAvail().y),
                                              false);
                            navigation(&state);
                            ImGui::EndChild();

                            ImGui::SameLine();
                        }

                        // Files and Folders
                        ImGui::BeginChild("Files and Folders",
                                          ImVec2(ImGui::GetContentRegionAvail().x,
                                                 ImGui::GetContentRegionAvail().y),
                                          false,
                                          ImGuiInputTextFlags_NoHorizontalScroll);
                        files_and_folders(&state);
                        ImGui::EndChild();

                        ImGui::EndTabItem();
                    }
                    if (!open) {
                        remove_tab(&state, i);
                    }
                }

                ImGui::EndTabBar();
            }

            bool init = false;
            static char search[PATH_MAX_SIZE];
            static char found[PATH_MAX_SIZE];
            if (!ImGui::IsAnyItemActive() &&
                 ImGui::IsKeyPressed(ImGuiKey_P) &&
                 ImGui::GetIO().KeyCtrl)
            {
                search[0] = '\0';
                found[0] = '\0';
                init = true;
                state.open_search = true;
            }

            if (state.open_search) {
                ImGui::OpenPopup("Search##Popup");
                state.open_search = false;
            }

            // @warning: not a good searcher, and not run in different thread
            // so it stalls the program
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Search##Popup",
                                       NULL,
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoSavedSettings))
            {
                static bool not_found = false;
                if (init) {
                    ImGui::SetKeyboardFocusHere();
                    not_found = false;
                }

                if (ImGui::InputText("##Search Input", search, PATH_MAX_SIZE, ImGuiInputTextFlags_EnterReturnsTrue)) {
                    found[0] = '\0';
                    win32_find_file(found, PATH_MAX_SIZE, state.tabs[state.current_tab].current_dir, search);
                    if (found[0] == '\0') not_found = true;
                    else not_found = false;
                }
                if (found[0] != '\0') {
                    if (button_with_icon(state.tex_file.id, found)) {
                        open_file_or_folder(&state, found, false);
                    }
                } else if (not_found) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                    ImGui::TextWrapped("File Not Found!");
                    ImGui::PopStyleColor();
                }
                if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            // ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }
        ImGui::End();

        // if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

        // -------- RENDER
        ImGui::Render();

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    save_settings(&state);
}
