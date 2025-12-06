//
// Created by Pavel on 06.12.2025.
//
#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <fstream>
#include <system_error>

const std::filesystem::path TEMP_DIR = std::filesystem::current_path() / "tmp";

int run_wget(const std::string& url, const std::filesystem::path& output_file) {
    std::string command = "wget --timeout=10 --tries=1 -O \""
        + output_file.string() + "\" \"" + url + "\"";
    return std::system(command.c_str());
}

void remove_file_and_temp_dir(const std::filesystem::path& temp_dir, const std::string& file_name) {
    try {
        std::filesystem::remove(TEMP_DIR / file_name);
        std::filesystem::remove(temp_dir);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error removing file: " << e.what() << std::endl;
    }
}

// Тест: проверка доступности ресурса
TEST(WgetTest, FetchHomepageBasic) {
    const std::string url = "http://example.com";
    std::string output_file_name = "test_output.html";
    const std::filesystem::path output_file = TEMP_DIR / output_file_name;

    //Create temp directory if doesnt exist
    std::filesystem::create_directories(TEMP_DIR);

    int exit_code = run_wget(url, output_file);

    EXPECT_EQ(exit_code, 0) << "wget failed with exit code " << exit_code;

    // Проверяем существование файла
    ASSERT_TRUE(std::filesystem::exists(output_file))
        << "Output file not created: " << output_file;

    // Проверяем размер файла
    auto file_size = std::filesystem::file_size(output_file);
    EXPECT_GT(file_size, 0u) << "Downloaded file is empty";

    // Очищаем временный файл
    remove_file_and_temp_dir(TEMP_DIR, output_file_name);
}

// Тест: проверка содержимого
TEST(WgetTest, VerifyContent) {
    const std::string url = "http://httpbin.org/html";
    std::string output_file_name = "content_test.htm";
    const std::filesystem::path output_file = TEMP_DIR / output_file_name;

    //Create temp directory if doesnt exist
    std::filesystem::create_directories(TEMP_DIR);

    int exit_code = run_wget(url, output_file);
    EXPECT_EQ(exit_code, 0);

    std::ifstream file(output_file);
    ASSERT_TRUE(file.is_open()) << "Failed to open downloaded file: " << output_file;

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    //TODO: CHECK AND REPLACE IF DIFFERENT CONTENT
    EXPECT_NE(content.find("<h1>Herman Melville - Moby-Dick</h1>"), std::string::npos)
        << "Expected title not found in response";

    remove_file_and_temp_dir(TEMP_DIR, output_file_name);
}