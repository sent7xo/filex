#ifndef APP_H

#define PATH_MAX_SIZE 256
#define WIN32_PATH_MAX_SIZE MAX_PATH

#define SETTINGS_FILENAME "settings.filex"

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
    uint id = 0;

    char current_dir[PATH_MAX_SIZE];
    Back_History back;

    int selected[SELECTED_MAX_COUNT];
    int selected_count = 0;

    bool editing = false;
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

#define GROUP_FOLDER_MAX_COUNT 32
struct Group {
    uint id = 0;
    char name[64];
    char location[PATH_MAX_SIZE];
    char folders[GROUP_FOLDER_MAX_COUNT][64];
    uint count = 0;

    Group *next = NULL;
};

#define TAB_MAX_COUNT 32
#define GROUP_MAX_COUNT 64
struct App_State {
    char start_path[64] = "C:\\";
    char wallpaper[PATH_MAX_SIZE];
    char exe_path[PATH_MAX_SIZE];


    // @note: since Tab struct contains a LOT of strings,
    // copying can be a problem (slow). this is the best
    // solution i came up with, maybe there's a better way?
    uint tab_gen_id = 1; // @note: 0 is invalid
    // uint tab_indices[TAB_MAX_COUNT];
    Tab tabs[TAB_MAX_COUNT];
    uint tab_count = 0;
    uint current_tab = -1;

    uint group_gen_id = 1;
    Group groups[GROUP_MAX_COUNT];
    // uint group_count;

    // char current_dir[PATH_MAX_SIZE];
    char favorites[32][PATH_MAX_SIZE];
    uint favorite_count = 0;
    int selected_fav_index = 0;

    char (*copy)[PATH_MAX_SIZE] = NULL;
    int copy_count = 0;
    bool cut = false;

    bool open_search = false;
    bool show_navigation_panel = true;
    bool show_hidden_files = true;
    bool show_tab_list_popup_button = true;
    bool list_view = false; // @todo: replace with layout
    bool light_mode = false;

    bool path_invalid = false;
    // Layout::Enum layout = Layout::ICONS;

    Texture tex_wallpaper;
    Texture tex_wallpaper0;
    Texture tex_wallpaper1;
    Texture tex_wallpaper2;
    Texture tex_wallpaper3;

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
    Texture tex_down;
    Texture tex_edit;
    Texture tex_search;
    Texture tex_info;

    Texture tex_back_dark;
    Texture tex_forward_dark;
    Texture tex_up_dark;
    Texture tex_down_dark;
    Texture tex_edit_dark;
    Texture tex_search_dark;
    Texture tex_info_dark;
    Texture tex_copy_dark;
    Texture tex_paste_dark;
    Texture tex_rename_dark;
    Texture tex_cut_dark;
    Texture tex_file_txt_dark;

    Texture tex_copy;
    Texture tex_paste;
    Texture tex_rename;
    Texture tex_cut;
    Texture tex_delete;
    Texture tex_favorite;

    // @todo: binary tree?
    // int selected[SELECTED_MAX_COUNT];
    // int selected_count = 0;

    // Back_History back;

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
