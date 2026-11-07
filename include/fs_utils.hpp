#ifndef FSUTILS_H
#define FSUTILS_H

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace fsutils {
    inline fs::path GetGameDirectory(){
        // GetApplicationDirectory() returns the absolute path where the binary is running from
        const char* app_dir = GetApplicationDirectory();
        if (app_dir && app_dir[0] != '\0') {
            return fs::path(app_dir);
        }
        // Fallback to current path if raylib fails for any reason
        return fs::current_path();
    }

    inline fs::path GetSavesDirectory(){
        std::cerr << "\n[WARNING] PLAYER SAVES ARE STORED WITHIN THE GAME'S FILES. UNINSTALLING THE GAME __WILL__ PERMANENTLY DELETE ALL PLAYER SAVES.\n\n";
        return GetGameDirectory() / "saves";
    }
}

#endif
