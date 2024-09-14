#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <stdexcept>
#include "filesystem.h"

class memory_mapped_file
{
    size_t size_{};
    char* data_{};
    bool exists_{ false };
    std::string filename_;

public:
    bool open_existing(const char* filename);
    bool open_new(const char* filename, size_t size);
    void fill(uint8_t filler);
    void set_size_and_close(size_t new_size);
    void close();
    ~memory_mapped_file() { close(); };

    inline std::string_view data() const { return { data_, size_ }; }
    inline char* rawdata() { return data_; }
    template <typename T> inline T* rawdata() { return reinterpret_cast<T*>(data_); }
    inline char* rawdata(size_t offset) { return data_ + offset; }
    template <typename T> inline T* rawdata(size_t offset) { return reinterpret_cast<T*>(data_ + offset); }
    inline bool exists() const { return exists_; }
    inline size_t size() const { return size_; }
};

bool memory_mapped_file::open_existing(const char* filename)
{
    if (!g_pFileSystem->FileExists(filename))
    {
        exists_ = false;
        return false;
    }

    FileHandle_t file = g_pFileSystem->Open(filename, "rb");
    if (!file)
    {
        exists_ = false;
        return false;
    }

    size_ = g_pFileSystem->Size(file);
    if (size_ == 0)
    {
        g_pFileSystem->Close(file);
        exists_ = false;
        return false;
    }

    data_ = new char[size_];
    int bytesRead = g_pFileSystem->Read(data_, static_cast<int>(size_), file);
    g_pFileSystem->Close(file);

    if (bytesRead != static_cast<int>(size_))
    {
        delete[] data_;
        data_ = nullptr;
        exists_ = false;
        throw std::runtime_error("Failed to read the entire file: " + std::string(filename));
    }

    filename_ = filename;
    exists_ = true;
    return true;
}

bool memory_mapped_file::open_new(const char* filename, size_t size)
{
    // Allocate a buffer of the specified size
    size_ = size;
    data_ = new char[size_];
    std::memset(data_, 0, size_);

    filename_ = filename;
    exists_ = true;
    return true;
}

void memory_mapped_file::fill(uint8_t filler)
{
    if (!exists_ || !data_)
        throw std::runtime_error("File not opened or data is null.");

    std::memset(data_, filler, size_);
}

void memory_mapped_file::set_size_and_close(size_t new_size)
{
    if (!exists_ || !data_)
        return;

    // Adjust the size if necessary
    if (new_size != size_)
    {
        char* new_data = new char[new_size];
        size_t copy_size = (new_size < size_) ? new_size : size_;
        std::memcpy(new_data, data_, copy_size);
        delete[] data_;
        data_ = new_data;
        size_ = new_size;
    }

    // Open the file for writing
    FileHandle_t file = g_pFileSystem->Open(filename_.c_str(), "wb");
    if (!file)
    {
        delete[] data_;
        data_ = nullptr;
        size_ = 0;
        exists_ = false;
        throw std::runtime_error("Failed to open file for writing: " + filename_);
    }

    int bytesWritten = g_pFileSystem->Write(data_, static_cast<int>(size_), file);
    g_pFileSystem->Close(file);

    if (bytesWritten != static_cast<int>(size_))
    {
        delete[] data_;
        data_ = nullptr;
        size_ = 0;
        exists_ = false;
        throw std::runtime_error("Failed to write the entire buffer to file: " + filename_);
    }

    close();
}

void memory_mapped_file::close()
{
    if (data_)
    {
        delete[] data_;
        data_ = nullptr;
    }
    exists_ = false;
    size_ = 0;
}
