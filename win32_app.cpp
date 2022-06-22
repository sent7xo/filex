// #include <shlobj_core.h>
// #include <shellapi.h>

bool get_files_and_folders (char *path,
                            char **dirs,
                            int *dir_count,
                            char **files,
                            int *file_count)
{
    WIN32_FIND_DATAA file_data;
    HANDLE file_handle = FindFirstFile(path, &file_data);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    defer { FindClose(file_handle); };
    do {
        // @note: skip $RECYCLE.BIN and non-ascii chars
        if (strcmp(file_data.cFileName, ".") == 0 ||
            strcmp(file_data.cFileName, "..") == 0 ||
            file_data.cFileName[0] == '$' ||
            strcmp(file_data.cFileName, "System Volume Information") == 0) continue;
        if (strlen(file_data.cFileName) > FILENAME_MAX_SIZE) continue;
        if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            assert(*dir_count < FILENAME_MAX_COUNT);
            strcpy_s(dirs[*(dir_count)++], MAX_PATH, file_data.cFileName);
        } else {
            assert(*file_count < FILENAME_MAX_COUNT);
            strcpy_s(files[*(file_count)++], MAX_PATH, file_data.cFileName);
        }
    } while(FindNextFile(file_handle, &file_data));

    return true;
}
