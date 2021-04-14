#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>
#include <algorithm>
#include <experimental/filesystem>

#include <Eigen/Dense>
typedef const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> ConstRowMatrixX3f;
typedef Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> RowMatrixX3f;
typedef Eigen::Matrix<float, Eigen::Dynamic, 9, Eigen::RowMajor> RowMatrixX9f;
typedef Eigen::Matrix<float, Eigen::Dynamic, 16, Eigen::RowMajor> RowMatrixX16f;

namespace utils {
namespace io{
    namespace fs = std::experimental::filesystem;

    inline bool pathExists(const fs::path &p, fs::file_status s = fs::file_status{}) {
        return fs::status_known(s) ? fs::exists(s) : fs::exists(p);
    }

    inline bool pathExists(const std::string &file_path, fs::file_status s = fs::file_status{}) {
        fs::path path(file_path);
        return pathExists(path);
    }

    inline bool checkExtension(const std::string &file_path, const std::string &ext) {
        return (fs::path(file_path).extension().string() == ext);
    }

    inline std::string getFileName(const std::string &file_path, bool with_ext = true) {
        fs::path path(file_path);
        if (!pathExists(path))
            throw std::runtime_error("ERROR: Path "+file_path+" doesn't exist!");

        if (with_ext)
            return path.filename().string();
        else
            return path.stem().string();
    }

    inline bool makeCleanFolder(const std::string &dir_path) {
        fs::path abs_dir = fs::absolute(dir_path);
        bool suc = false;
        if (!pathExists(abs_dir) && pathExists(abs_dir.parent_path())) {
            suc = fs::create_directory(abs_dir);
        }
        else {
            for (const auto &entry : fs::directory_iterator(abs_dir))
                fs::remove_all(entry.path());
            suc = true;
        }
        return suc;
    }
};
};


#endif //UTILS_H
