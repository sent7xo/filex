Texture init_texture(char *path, int type = GL_RGBA)
{
    Texture tex;

    int channels = 0; // ignore
    uchar *image = stbi_load(path, &tex.width, &tex.height, &channels, 0);
    if (!image) {
        printf("Failed to load texture: %s\n", path);
        assert(false);
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

bool file_button (ImTextureID id, char *text, ImVec2 image_size)
{
    bool result = false;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 button_size = ImVec2(image_size.x, image_size.y + 30);
    ImVec2 padding = style.FramePadding;

    result = ImGui::InvisibleButton(text, button_size + padding * 2);

    bool hover = ImGui::IsItemHovered();
    bool click = ImGui::IsItemClicked(0);
    ImU32 col = ImGui::GetColorU32((hover && click) ? ImGuiCol_ButtonActive :
                                   hover ? ImGuiCol_ButtonHovered :
                                   ImGuiCol_Button,
                                   9999);
    ImVec2 text_size = ImGui::CalcTextSize(text, NULL, false, button_size.x - padding.x);

    if (hover) {
        ImVec2 min = draw_list->GetClipRectMin();
        ImVec2 max = draw_list->GetClipRectMax();
        ImGui::SetNextWindowFocus();
        ImGui::SetNextWindowSize(ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(0);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("Folder Name Hover", NULL, flags);
        draw_list = ImGui::GetWindowDrawList();
        draw_list->PushClipRect(min, max);
    }

    draw_list->AddRectFilled(p,
                             ImVec2(p.x + button_size.x + padding.x * 2,
                                    p.y + image_size.y + text_size.y + padding.y * 2),
                             col,
                             ImClamp((float)ImMin(padding.x, padding.y), 0.0f, g.Style.FrameRounding));

    float offset = padding.x;
    draw_list->AddImage(id,
                        p + padding + ImVec2(offset, 0),
                        p + image_size - padding + ImVec2(offset, 0));
    ImVec4 clipping = ImVec4(p.x + padding.x,
                             p.y + padding.y,
                             p.x + button_size.x - padding.x,
                             p.y + button_size.y - padding.y);

    draw_list->AddText(g.Font,
                       g.FontSize,
                       ImVec2(p.x + padding.x, p.y + image_size.y),
                       ImGui::GetColorU32(ImGuiCol_Text),
                       text,
                       NULL,
                       button_size.x - padding.x,
                       hover ? NULL : &clipping);

    if (hover) {
        draw_list->PopClipRect();
        ImGui::End();
    }

    return result;
}

bool win32_file_exists (char *path)
{
    DWORD attrib = GetFileAttributes(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
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
    Back_History *back = &state->back;
}

void add_back (App_State *state)
{
    Back_History *back = &state->back;

    assert(back->cursor < back->count);
    back->count = back->cursor + 1;

    int index = (back->start + back->count) % HISTORY_MAX_COUNT;
    strcpy_s(back->history[index], state->current_dir);

    if (back->count < HISTORY_MAX_COUNT) {
        back->cursor++;
        back->count++;
    } else {
        back->start = (back->start + 1) % HISTORY_MAX_COUNT;
    }
    printf("\"%s\" start: %d, cursor: %d, count: %d\n", back->history[index], back->start, back->cursor, back->count);
}

void go_back (App_State *state)
{
    Back_History *back = &state->back;
    if (back->cursor <= 0) return;

    back->cursor--;
    int index = (back->start + back->cursor) % HISTORY_MAX_COUNT;
    strcpy_s(state->current_dir, back->history[index]);
}

void go_forward (App_State *state)
{
    Back_History *back = &state->back;
    if (back->cursor >= back->count - 1) return;

    back->cursor++;
    int index = (back->start + back->cursor) % HISTORY_MAX_COUNT;
    strcpy_s(state->current_dir, back->history[index]);
}

// @todo: "\Handbrake" is illegal!!! must be "D:\Handbrake"
void path_bar (App_State *state)
{
    char *current_dir = state->current_dir;

    ImVec2 button_size= ImVec2(20, 20);
    ImVec2 image_button_size = ImVec2(14, 14);

    if (ImGui::ImageButton(state->tex_back.id, image_button_size)) {
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
    ImGui::SameLine();

    float start_x = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x;
    static bool editing = false;
    if (editing) {
        bool init = false;
        if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
            ImGui::SetKeyboardFocusHere();
            init = true;
        }
        if (ImGui::InputText("##Path Input",
                             current_dir,
                             PATH_MAX_SIZE,
                             ImGuiInputTextFlags_EnterReturnsTrue |
                             ImGuiInputTextFlags_AutoSelectAll) ||
            (ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered()))
        {
            editing = false;

            // @note: if the path is a valid folder (no trailing word after slash)
            if (get_last_slash(current_dir)[0] == '\0') {
                add_back(state);
            }
        }
        if (init) {
            // @note: clear selection when we first focus
            ImGuiID id = ImGui::GetItemID();
            if (ImGuiInputTextState *state = ImGui::GetInputTextState(id)) {
                state->ClearSelection();
            }
        }
    } else {
        ImGui::SameLine(start_x, 0);

        ImGui::PushStyleColor(ImGuiCol_Button, state->theme.path_button);
        char temp_path[256];
        strcpy_s(temp_path, current_dir);

        char *folder, *ctx;
        folder = strtok_s(temp_path, "\\", &ctx);
        while (folder) {
            char *next_folder = strtok_s(NULL, "\\", &ctx);
            if (!next_folder) {
                ImGui::PopStyleColor();
            }
            if (ImGui::Button(folder)) {
                char *backslash = folder;
                while (*backslash) backslash++;
                current_dir[backslash - temp_path + 1] = '\0';
                printf("%s\n", current_dir);
                add_back(state);
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                editing = true;
            }
            ImGui::SameLine(0, 0);

            folder = next_folder;
            if (folder) {
                ImGui::Text("|");
                ImGui::SameLine(0, 0);
            }
        }
        ImGui::SameLine(0, 0);

        // Edit
        ImGui::PushStyleColor(ImGuiCol_Button, state->theme.path_button);
        if (ImGui::ImageButton(state->tex_edit.id, image_button_size)) {
            editing = true;
        }
        ImGui::PopStyleColor();
    }

}

void set_style (App_State *state)
{
    Theme *theme = &state->theme;

    theme->path_button = ImVec4(0, 0, 0, 0);
    // theme->path_button_hover;
    // theme->path_button_click;

    theme->files_button = ImVec4(0, 0, 0, 0);
    theme->files_button_hover = ImGui::GetStyle().Colors[ImGuiCol_Button];
    theme->files_button_click = ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered];

    theme->files_selected_button = theme->files_button_click;
    theme->files_selected_button_hover = theme->files_button_click;
    // theme->files_selected_button_click;
}

#define FILENAME_MAX_COUNT 512
#define FILENAME_MAX_SIZE 64
// @todo: cache this, and update every so often
void files_and_folders (App_State *state)
{
    char *current_dir = state->current_dir;

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::PushStyleColor(ImGuiCol_Button, state->theme.files_button);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, state->theme.files_button_hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, state->theme.files_button_click);

    defer {
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    };

    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s*", current_dir);

    // @todo: put these in platform specific *_app.cpp
    WIN32_FIND_DATAA file_data;
    HANDLE file_handle = FindFirstFile(path, &file_data);
    if (file_handle == INVALID_HANDLE_VALUE ||
        get_last_slash(path) == path) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
        ImGui::TextWrapped("Invalid Path!");
        ImGui::PopStyleColor();
        ImGui::TextWrapped("Might be because the path doesn't exist or Filex doesn't have the permission to access it.");
        return;
    }
    defer { FindClose(file_handle); };

    char folders[FILENAME_MAX_COUNT][FILENAME_MAX_SIZE];
    uint folder_count = 0;

    char files[FILENAME_MAX_COUNT][FILENAME_MAX_SIZE];
    uint file_count = 0;

    do {
        if (strcmp(file_data.cFileName, ".") == 0 ||
            strcmp(file_data.cFileName, "..") == 0) continue;
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
        state->selected_count = 0;

        strcpy_s(prev_path, current_dir);
    }

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec2 button_size = ImVec2(80, 80);
    float window_visible = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    bool is_selecting = false;
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
        for (int j = 0; j < state->selected_count; j++) {
            if (state->selected[j] != i) continue;

            ImGui::PushStyleColor(ImGuiCol_Button, state->theme.files_selected_button);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  state->theme.files_selected_button_hover);
            found = j;
            break;
        }

        if (state->list_view) {
            ImGui::Button(label, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0));
        } else {
            ImVec2 p = ImGui::GetCursorScreenPos();
            if (is_folder) {
                // ImGui::ImageButton(state->tex_folder.id, button_size);
                file_button(state->tex_folder.id, folders[index], button_size);
            } else {
                ImTextureID id = state->tex_file.id;
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

                file_button(id, text, button_size);
                // ImGui::ImageButton(state->tex_file.id, button_size);
            }

            // @note: using BeginChild mess this up, since we already know button
            // size, just use it instead
            float last_button = ImGui::GetItemRectMax().x;
            float next_button = last_button + style.ItemSpacing.x + button_size.x;
            // float next_button = p.x + button_size.x + style.ItemSpacing.x + button_size.x;
            if (next_button < window_visible) ImGui::SameLine();
        }

        // @note: since ImGui::Button is delayed, we put it here
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            if (ImGui::GetIO().KeyShift) {
                if (found > -1) {
                    state->selected[found] = state->selected[state->selected_count-- - 1];
                } else {
                    state->selected[state->selected_count++] = i;
                }
            } else {
                state->selected_count = 0;
                state->selected[state->selected_count++] = i;
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
            strncpy_s(temp_path, current_dir, last_backslash_count);

            if (is_folder) {
                snprintf(current_dir, PATH_MAX_SIZE, "%s%s\\", temp_path, folders[index]);
                add_back(state);
            } else {
                strcat_s(temp_path, PATH_MAX_SIZE, files[index]);
                ShellExecute(NULL, NULL, temp_path, NULL, NULL, SW_SHOW);
            }
        }

        if (found > -1) {
            ImGui::PopStyleColor(2);
        }
    }

    // @note: klik di luar buttons
    if (ImGui::IsMouseClicked(0) && !is_selecting) {
        state->selected_count = 0;
    }
}

void navigation (App_State *state)
{
    char *current_dir = state->current_dir;

    static bool show_quick_access = true;
    static bool show_this_pc = true;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(show_quick_access ? "v Quick Access" : "> Quick Access")) show_quick_access = !show_quick_access;
    ImGui::PopStyleColor();
    if (show_quick_access) {
        for (uint i = 0; i < state->quick_access_count; i++) {
            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();
            ImGui::Button(state->quick_access[i]);
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
        KNOWNFOLDERID folder_id[] = {
            FOLDERID_Documents,
            FOLDERID_Downloads,
            FOLDERID_Music,
            FOLDERID_Pictures,
            FOLDERID_Videos,
        };
        assert(IM_ARRAYSIZE(folder_name) == IM_ARRAYSIZE(folder_id));

        for (int i = 0; i < IM_ARRAYSIZE(folder_name); i++) {
            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();
            if (ImGui::Button(folder_name[i])) {
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
                snprintf(drive, 64, "[%c] %s", *drive_letter, drive_name);
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

void app (SDL_Window *window)
{
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigInputTextCursorBlink = false;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0;
    style.FrameRounding = style.GrabRounding = 12;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);

    App_State state;
    int current_dir_len = IM_ARRAYSIZE(state.current_dir);

    strcpy_s(state.current_dir, "D:\\");
    add_back(&state);

    strcpy_s(state.quick_access[state.quick_access_count++], "Recycle Bin");
    strcpy_s(state.quick_access[state.quick_access_count++], "KULIAH");
    strcpy_s(state.quick_access[state.quick_access_count++], "Test");

    set_style(&state);

    state.tex_folder = init_texture("D:\\programming\\c_cpp\\filex\\data\\FOLDERS.png");
    state.tex_file = init_texture("D:\\programming\\c_cpp\\filex\\data\\file.png");
    state.tex_edit = init_texture("D:\\programming\\c_cpp\\filex\\data\\edit.png");
    state.tex_back = init_texture("D:\\programming\\c_cpp\\filex\\data\\back.png");
    state.tex_forward = init_texture("D:\\programming\\c_cpp\\filex\\data\\forward.png");
    state.tex_up = init_texture("D:\\programming\\c_cpp\\filex\\data\\up.png");
    state.tex_file_exe = init_texture("D:\\programming\\c_cpp\\filex\\data\\exe.png");
    state.tex_file_txt = init_texture("D:\\programming\\c_cpp\\filex\\data\\txt.png");
    state.tex_file_mp3 = init_texture("D:\\programming\\c_cpp\\filex\\data\\mp3.png");
    state.tex_file_mp4 = init_texture("D:\\programming\\c_cpp\\filex\\data\\mp4.png");
    state.tex_file_image = init_texture("D:\\programming\\c_cpp\\filex\\data\\image.png");
    state.tex_file_zip = init_texture("D:\\programming\\c_cpp\\filex\\data\\zip.png");
    state.tex_file_word = init_texture("D:\\programming\\c_cpp\\filex\\data\\word.png");

    state.tex_wallpaper = init_texture("D:\\programming\\c_cpp\\filex\\data\\wallpaper0.png");

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
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("Main", NULL, main_window_flags);
        {
            // Background
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                ImVec2 p = ImGui::GetWindowPos();
                ImVec2 s = ImGui::GetWindowSize();
                draw_list->PushClipRectFullScreen();
                draw_list->AddImage(state.tex_wallpaper.id,
                                    p,
                                    p + s);
                draw_list->PopClipRect();
            }

            // Menu
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New", "CTRL+Z")) {}
                    ImGui::Separator();

                    if (ImGui::MenuItem("Settings", "CTRL+O")) {
                        printf("Heyyy\n");
                    }
                    ImGui::Separator();

                    if (ImGui::MenuItem("New", "CTRL+Z")) {}
                    if (ImGui::MenuItem("Exit", "CTRL+Z")) {}
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
                    if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
                    ImGui::Separator();
                    if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                    if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                    if (ImGui::MenuItem("Paste", "CTRL+V")) {}
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Settings")) {
                    ImGui::MenuItem("Files and Folders", NULL, false, false);
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
                ImGui::EndMainMenuBar();
            }

            static ImVector<int> active_tabs;
            static int next_tab_id = 0;
            if (active_tabs.Size == 0) {
                active_tabs.push_back(next_tab_id++);
            }

            // Tabs
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs |
                                             ImGuiTabBarFlags_Reorderable |
                                             ImGuiTabBarFlags_FittingPolicyScroll |
                                             ImGuiTabBarFlags_NoTabListScrollingButtons;
            if (state.show_tab_list_popup_button) tab_bar_flags |= ImGuiTabBarFlags_TabListPopupButton;

            if (ImGui::BeginTabBar("MainTabBar", tab_bar_flags)) {
                if (ImGui::TabItemButton("+",
                                         ImGuiTabItemFlags_Trailing |
                                         ImGuiTabItemFlags_NoTooltip))
                {
                    active_tabs.push_back(next_tab_id++);
                }

                for (int i = 0; i < active_tabs.Size;) {
                    bool open = true;

                    char tab_name[32];

                    // @todo: we can't change tab's name willynilly
#if 1
                    char *last_folder = state.current_dir;
                    char *temp = last_folder;
                    while (*temp) {
                        if (*temp == '\\') {
                            if (*(temp + 1) == '\0') {
                                char temp_name[32];
                                snprintf(temp_name, 16, "%.*s", (int) (temp - last_folder), last_folder);
                                snprintf(tab_name, 32, "%s###tab%d", temp_name, i);
                                break;
                            } else {
                                last_folder = temp + 1;
                            }
                        }
                        temp++;
                    }
#else
                    snprintf(tab_name, 16, "%d", i);
#endif

                    if (ImGui::BeginTabItem(tab_name, &open)) {
                        path_bar(&state);
                        ImGui::Separator();

                        ImGui::BeginChild("Navigation",
                                          ImVec2(ImGui::GetContentRegionAvail().x * 0.2f,
                                                 ImGui::GetContentRegionAvail().y),
                                          false);
                        navigation(&state);
                        ImGui::EndChild();

                        ImGui::SameLine();

                        ImGui::BeginChild("Files and Folders",
                                          ImVec2(ImGui::GetContentRegionAvail().x,
                                                 ImGui::GetContentRegionAvail().y),
                                          true);
                        files_and_folders(&state);
                        ImGui::EndChild();

                        ImGui::EndTabItem();
                    }
                    if (!open) {
                        active_tabs.erase(active_tabs.Data + i);
                    } else {
                        i++;
                    }
                }

                ImGui::EndTabBar();
            }

            // ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }
        ImGui::End();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // -------- RENDER
        ImGui::Render();

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }
}
