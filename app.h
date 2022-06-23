#ifndef APP_H

#define PATH_MAX_SIZE 256
#define WIN32_PATH_MAX_SIZE MAX_PATH

struct Theme {
    ImVec4 path_button;
    ImVec4 path_button_hover;
    ImVec4 path_button_click;

    // @note: files and folders
    ImVec4 files_button;
    ImVec4 files_button_hover;
    ImVec4 files_button_click;

    ImVec4 files_selected_button;
    ImVec4 files_selected_button_hover;
    ImVec4 files_selected_button_click;

    ImVec4 nav_button;
    ImVec4 nav_button_hover;
    ImVec4 nav_button_click;
};

#define HISTORY_MAX_COUNT 32
// @note: back/forward, circular buffer
struct Back_History {
    int cursor = -1;

    char history[HISTORY_MAX_COUNT][PATH_MAX_SIZE];
    int start = 0;
    int count = 0;
};

// @todo: undo/redo
struct Action {
    enum Type {
        DELETED,
        MOVE,
        PASTE,
        RENAME,
    };
    Type type;

    char path[PATH_MAX_SIZE];
    char path_target[PATH_MAX_SIZE]; // @note: if needed
};

struct Undo_History {
    int cursor = -1;

    Action history[HISTORY_MAX_COUNT];
    int count = 0;
};
#define SELECTED_MAX_COUNT 1024

struct Tab {
    uint id;

    char current_dir[PATH_MAX_SIZE];
    Back_History back;

    int selected[SELECTED_MAX_COUNT];
    int selected_count = 0;
};

struct Texture {
    ImTextureID id;
    int width, height;
};

namespace Layout {
    enum Enum {
        ICONS,
        LIST,
        COUNT,
    };
};

struct App_State {
    char exe_path[PATH_MAX_SIZE];

    uint tab_gen_id = 1; // @note: 0 is invalid
    uint tab_indices[32];
    Tab tabs[32];
    uint tab_count = 0;

    char current_dir[PATH_MAX_SIZE];

    char quick_access[32][PATH_MAX_SIZE];
    uint quick_access_count = 0;

    bool show_navigation_panel = true;
    bool show_hidden_files = true;
    bool show_tab_list_popup_button = true;
    bool list_view = false; // @todo: replace with layout
    // Layout::Enum layout = Layout::ICONS;

    Texture tex_wallpaper;

    Texture tex_folder;
    Texture tex_file;

    Texture tex_file_exe;
    Texture tex_file_txt;
    Texture tex_file_mp3;
    Texture tex_file_mp4;
    Texture tex_file_image;
    Texture tex_file_zip;
    Texture tex_file_word;

    Texture tex_back;
    Texture tex_forward;
    Texture tex_up;
    Texture tex_edit;

    Texture tex_copy;
    Texture tex_paste;
    Texture tex_rename;
    Texture tex_cut;
    Texture tex_delete;

    // @todo: binary tree?
    int selected[SELECTED_MAX_COUNT];
    int selected_count = 0;

    Back_History back;

    Theme theme;
};

bool file_exists (char *path);
bool get_files_and_folders (char *path,
                            char *dirs,
                            uint *dir_count,
                            char *files,
                            uint *file_count);

#define APP_H
#endif
