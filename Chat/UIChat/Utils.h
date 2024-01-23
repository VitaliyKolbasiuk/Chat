#pragma once

#include <iostream>
#include <random>
#include <string>
#include <array>
#include <sstream>
#include <iomanip>

template<size_t N>
void generateRandomKey(std::array<uint8_t, N>& key)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(65, 90);  // ASCII range for uppercase letters

    for (size_t i = 0; i < N; ++i) {
        key[i] = static_cast<uint8_t>(distribution(generator));
    }
}

template<size_t N>
std::string arrayToHexString(const std::array<uint8_t, N>& arr)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& element : arr) {
        ss << std::setw(2) << static_cast<int>(element);
    }
    return ss.str();
}

template<size_t N>
std::array<uint8_t, N> parseHexString(const std::string& hexString) {
    std::array<uint8_t, N> byteArr;

    // Ensure that the hex string has an even number of characters
    if (hexString.length() % 2 != 0) {
        std::cout << "Invalid hex string packetLength." << std::endl;
        return byteArr;
    }

    for (std::size_t i = 0; i < hexString.length(); i += 2) {
        std::string byteString = hexString.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        byteArr[i / 2] = byte;
    }

    return byteArr;
}

inline uint64_t currentUtc()
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    auto utcTime = std::chrono::time_point_cast<std::chrono::seconds>(now);

    auto seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(utcTime.time_since_epoch());

    return seconds_since_epoch.count();
}


inline void parseUtcTime(uint64_t utcTime, int& year, int& month, int& day, int& hour, int& minute, int& second)
{
    std::time_t time = static_cast<std::time_t>(utcTime);
    std::tm* tmInfo = std::gmtime(&time);

    year = tmInfo->tm_year + 1900;
    month = tmInfo->tm_mon + 1;
    day = tmInfo->tm_mday;
    hour = tmInfo->tm_hour;
    minute = tmInfo->tm_min;
    second = tmInfo->tm_sec;
}

