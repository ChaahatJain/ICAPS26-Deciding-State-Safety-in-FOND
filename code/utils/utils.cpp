//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <cmath>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "utils.h"
#include "../exception/plaja_exception.h"

/*
#include <unistd.h>
std::string get_current_dir() {
    char buf[FILENAME_MAX];
    return std::string(getcwd(buf, FILENAME_MAX));
}
*/

namespace PLAJA_UTILS {

    /* Hashes. */
    const std::hash<bool> hashBool;
    const std::hash<int> hashInteger;
    const std::hash<double> hashFloating;
    const std::hash<std::string> hashString;

    /******************************************************************************************************************/

    std::string extract_directory_path(const std::string& path_to_file) {
        auto ref = path_to_file.find_last_of('/');
        if (ref == std::string::npos) {
            return "./";
        } else {
            return path_to_file.substr(0, ref + 1); // increment to *include* '/' position
        }
    }

    std::string extract_filename(const std::string& path_to_file, bool keep_extension) {
        auto ref = path_to_file.find_last_of('/');
        std::string file_name = ref == std::string::npos ? path_to_file : path_to_file.substr(ref + 1); // Increment to *exclude* '/' position.
        if (keep_extension) {
            return file_name;
        } else {
            auto ref_extension = file_name.find_last_of('.');
            return (ref_extension == std::string::npos) ? file_name : file_name.substr(0, ref_extension);
        }
    }

    std::string extract_extension(const std::string& path_to_file) {
        auto ref_extension = path_to_file.find_last_of('.');
        return (ref_extension == std::string::npos) ? PLAJA_UTILS::emptyString : path_to_file.substr(ref_extension, path_to_file.size());
    }

    std::string remove_extension(const std::string& path_to_file) {
        return extract_directory_path(path_to_file) + extract_filename(path_to_file, false);
    }

    std::string join_path(const std::string& prefix, const std::vector<std::string>& sub_paths) {
        std::filesystem::path path(prefix);
        for (const auto& sub_path: sub_paths) {
            path = path / std::filesystem::path(sub_path);
        }
        return path.string();
    }

    bool is_relative_path(const std::string& path_to_file) {
        const std::filesystem::path path(path_to_file);
        return path.is_relative();
    }

    std::string generate_relative_path(const std::string& path_to_file, const std::string& path_to_base) { // NOLINT(bugprone-easily-swappable-parameters)
        const std::filesystem::path base(path_to_base);
        const std::filesystem::path path(path_to_file);
        return std::filesystem::relative(path, base).generic_string();
    }

    void make_directory(const std::string& path_to_dir) { std::filesystem::create_directory(path_to_dir); }

    bool file_exists(const std::string& file_name) {
        const std::ifstream f(file_name);
        return f.good();
    }

    void copy_file(const std::string& path_to_file, const std::string& path_to_copy) { std::filesystem::copy_file(std::filesystem::path(path_to_file), std::filesystem::path(path_to_copy)); }

    void write_to_file(const std::string& filename, const std::string& content, bool trunc) { // NOLINT(bugprone-easily-swappable-parameters)

        if (filename.empty()) {  // Empty string is reserved for std::cout.
            print(content);
            return;
        }

        std::ofstream out_file(filename, trunc ? std::ios::trunc : std::ios::app);
        if (out_file.fail()) { throw PlajaException(PLAJA_EXCEPTION::fileNotFoundString + filename); }
        out_file << content;
        out_file.close();

    }

    std::stringstream file_to_ss(const std::string& file_name) {
        const std::ifstream f(file_name);
        if (f.fail()) { throw PlajaException(PLAJA_EXCEPTION::fileNotFoundString + file_name); }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss;
    }

    /******************************************************************************************************************/

    double precision_to_tolerance(unsigned int precision) { return std::pow(0.1, precision); } // NOLINT(*-avoid-magic-numbers)

    extern unsigned int tolerance_to_precision(double tolerance) { return -PLAJA_UTILS::cast_numeric<unsigned int>(std::round(std::log10(tolerance))); }

    // https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values [October 2020]
    template<typename T>
    std::string to_string_with_precision_aux(const T value, const unsigned int precision, bool trailing_zeros) {

        std::ostringstream out;
        out.precision(precision);
        out << std::fixed << value;

        if (trailing_zeros) { return out.str(); }

        /* Remove trailing zeros. */
        auto rlt = out.str();
        return rlt.erase(rlt.find_last_not_of('0') + 1, std::string::npos);

    }

    template<typename T>
    std::string to_string_with_precision_aux(const T value, const double precision, bool trailing_zeros) {
        PLAJA_ASSERT(0 < precision and precision < 1)
        return to_string_with_precision_aux(value, PLAJA_UTILS::cast_numeric<unsigned int>(-std::floor(std::log10(precision))), trailing_zeros);
    }

    [[maybe_unused]] std::string to_string_with_precision(const float value, const unsigned int precision, bool trailing_zeros) { return to_string_with_precision_aux<float>(value, precision, trailing_zeros); }

    [[maybe_unused]] std::string to_string_with_precision(const float value, const double precision, bool trailing_zeros) { return to_string_with_precision_aux<float>(value, precision, trailing_zeros); }

    std::string to_string_with_precision(const double value, const unsigned int precision, bool trailing_zeros) { return to_string_with_precision_aux<double>(value, precision, trailing_zeros); }

    std::string to_string_with_precision(const double value, const double precision, bool trailing_zeros) { return to_string_with_precision_aux<double>(value, precision, trailing_zeros); }

    std::string to_red_string(const std::string& str) {
        std::stringstream ss;
        ss << PLAJA_UTILS::redColorStr << str << PLAJA_UTILS::resetColorStr;
        return ss.str();
    }

    std::string to_green_string(const std::string& str) {
        std::stringstream ss;
        ss << PLAJA_UTILS::greenColorStr << str << PLAJA_UTILS::resetColorStr;
        return ss.str();
    }

    bool contains(const std::string& str, const std::string& sub_str) { return str.find(sub_str) != std::string::npos; }

    std::string erase(std::string str, const std::string& sub_str, int max_number_of_erase) { // NOLINT(bugprone-easily-swappable-parameters)
        auto sub_str_size = sub_str.size();
        std::size_t index; // NOLINT(*-init-variables)
        while ((index = str.find(sub_str)) != std::string::npos) {
            str.erase(index, sub_str_size);
            /* Here, if max_number_of_erase (remaining) is initially negative, we erase arbitrarily often. */
            if (--max_number_of_erase == 0) { break; }
        }
        return str;
    }

    std::string erase_all(std::string str, const std::string& sub_str) { return erase(std::move(str), sub_str, -1); }

    std::string replace(std::string str, const std::string& sub_str_old, const std::string& sub_str_new, int max_number_of_subs) { // NOLINT(bugprone-easily-swappable-parameters)
        auto sub_str_size = std::min(sub_str_old.size(), sub_str_new.size());
        std::size_t index; // NOLINT(*-init-variables)
        while ((index = str.find(sub_str_old)) != std::string::npos) {
            str.replace(index, sub_str_size, sub_str_new);
            /* Here, if max_number_of_replace (remaining) is initially negative, we erase arbitrarily often. */
            if (--max_number_of_subs == 0) { break; }
        }
        return str;
    }

    std::string replace_all(std::string str, const std::string& sub_str_old, const std::string& sub_str_new) { return replace(std::move(str), sub_str_old, sub_str_new, -1); }

    std::vector<std::string> split_into(const std::string& str, const std::string& delimiter, int max_number_of_splits) { // NOLINT(bugprone-easily-swappable-parameters)
        std::string str_tmp = str;
        auto delimiter_size = delimiter.size();
        std::vector<std::string> split_list;
        // Adapted from: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c#14266139 [April 2020]
        std::size_t index; // NOLINT(*-init-variables)
        while ((index = str_tmp.find(delimiter)) != std::string::npos) {
            split_list.emplace_back(str_tmp.substr(0, index));
            str_tmp.erase(0, index + delimiter_size);
            /* Here, if max_number_of_splits (remaining) is initially negative, we split arbitrarily often. */
            if (--max_number_of_splits == 0) { break; }
        }
        if (not str_tmp.empty()) { split_list.push_back(std::move(str_tmp)); }
        return split_list;
    }

    std::vector<std::string> split(const std::string& str, const std::string& delimiter) { return split_into(str, delimiter, -1); }

    std::string string_f(const char* format, ...) {
        constexpr std::size_t buffer_size = 10000;
        va_list arg_list;
        va_start(arg_list, format);
        //
        char buffer[buffer_size];
#ifdef NDEBUG
        vsprintf(buffer, format, arg_list);
#else
        auto rlt = vsnprintf(buffer, buffer_size, format, arg_list);
        PLAJA_ASSERT(0 <= rlt and rlt < buffer_size)
#endif

        //
        va_end(arg_list);
        return { buffer };
    }

    char** strings_to_char_array(const std::vector<std::string>& str_vec) {
        char** char_array = new char* [str_vec.size()];

        std::size_t index = 0;
        for (const std::string& str_tmp: str_vec) {
            char_array[index] = new char[str_tmp.size() + 1];
            std::copy(str_tmp.begin(), str_tmp.end(), char_array[index]);
            char_array[index][str_tmp.size()] = '\0';
            ++index;
        }

        return char_array;
    }

    std::vector<std::string> char_array_to_strings(std::size_t size, char** char_array) {
        std::vector<std::string> str_vec;
        str_vec.reserve(size);

        for (std::size_t index = 0; index < size; ++index) {
            str_vec.emplace_back(reinterpret_cast<const char*>(char_array[index]));
        }

        return str_vec;
    }

    extern void delete_char_array(std::size_t size, char** char_array) {

        for (std::size_t index = 0; index < size; ++index) {
            delete[] char_array[index];
        }

        delete[] char_array;
    }

    // String::String(const char* str, const std::basic_string<char, std::char_traits<char>, std::allocator<char>>::allocator_type& alloc) noexcept try : basic_string(str, alloc) {} catch(...) { std::terminate(); }

    const std::string emptyString; // NOLINT(cert-err58-cpp)
    const std::string spaceString(" "); // NOLINT(cert-err58-cpp)
    const std::string lineBreakString("\n"); // NOLINT(cert-err58-cpp)
    const std::string tabString("\t"); // NOLINT(cert-err58-cpp)
    const std::string commaString(","); // NOLINT(cert-err58-cpp)
    const std::string dotString("."); // NOLINT(cert-err58-cpp)
    const std::string semicolonString(";"); // NOLINT(cert-err58-cpp)
    const std::string colonString(":"); // NOLINT(cert-err58-cpp)
    const std::string slashString("/"); // NOLINT(cert-err58-cpp)
    const std::string dashString("-"); // NOLINT(cert-err58-cpp)
    const std::string underscoreString("_"); // NOLINT(cert-err58-cpp)
    const std::string quoteString("\""); // NOLINT(cert-err58-cpp)
    const std::string equalString("="); // NOLINT(cert-err58-cpp)
    const std::string lParenthesisString("("); // NOLINT(cert-err58-cpp)
    const std::string rParenthesisString(")"); // NOLINT(cert-err58-cpp)

    const std::string resetColorStr("\033[0m"); // NOLINT(cert-err58-cpp) // https://stackoverflow.com/questions/9158150/colored-output-in-c [November 2022]
    const std::string redColorStr("\033[31m"); // NOLINT(cert-err58-cpp)
    const std::string greenColorStr("\033[32m"); // NOLINT(cert-err58-cpp)

    /******************************************************************************************************************/

    void print(const std::string& str) { std::cout << str; }

    void print_line(const std::string& str) { std::cout << str << lineBreakString; }

}