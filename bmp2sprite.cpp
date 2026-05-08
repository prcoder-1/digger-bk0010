#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>

// BMP file header (14 bytes)
#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

// BMP info header (40 bytes)
struct BMPInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

bool readBMP(const std::string& filename, int& width, int& height, std::vector<unsigned char>& rgbData) {
    std::ifstream bmpFile(filename, std::ios::binary);
    if (!bmpFile) {
        std::cerr << "Could not open BMP file\n";
        return false;
    }

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    bmpFile.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    bmpFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (fileHeader.bfType != 0x4D42) { // "BM"
        std::cerr << "Not a BMP file\n";
        return false;
    }
    if (infoHeader.biBitCount != 24) {
        std::cerr << "Only 24-bit BMP supported\n";
        return false;
    }
    if (infoHeader.biCompression != 0) {
        std::cerr << "Compressed BMP not supported\n";
        return false;
    }

    width = infoHeader.biWidth;
    height = infoHeader.biHeight;

    int rowSize = (3 * width + 3) & (~3);
    int dataSize = rowSize * height;
    std::vector<unsigned char> rowData(dataSize);

    bmpFile.seekg(fileHeader.bfOffBits, std::ios::beg);
    bmpFile.read(reinterpret_cast<char*>(rowData.data()), dataSize);

    rgbData.resize(width * height * 3);

    // BMP stores pixels bottom-up
    for (int y = 0; y < height; ++y) {
        int rowStart = y * rowSize;
        for (int x = 0; x < width; ++x) {
            int bmpIndex = rowStart + x * 3;
            int rgbIndex = ((height - 1 - y) * width + x) * 3;

            // BMP pixel format: B, G, R
            unsigned char b = rowData[bmpIndex];
            unsigned char g = rowData[bmpIndex + 1];
            unsigned char r = rowData[bmpIndex + 2];

            rgbData[rgbIndex] = r;
            rgbData[rgbIndex + 1] = g;
            rgbData[rgbIndex + 2] = b;
        }
    }

    return true;
}

void writeCHeader(const std::string& filename, int width, int height, const std::vector<unsigned char>& rgbData) {
    std::ofstream headerFile(filename);
    if (!headerFile) {
        std::cerr << "Could not open output header file\n";
        return;
    }
    
    int byte_width = width / 4;

    headerFile << "const uint8_t image[" << height << "][" << byte_width << "] =" << std::endl << "\t{" << std::endl << "\t\t";

    int shift = 0;
    int count = 0;
    int byte = 0;
    for (size_t i = 0; i < rgbData.size(); i += 3) {
        unsigned char r = rgbData[i];
        unsigned char g = rgbData[i+1];
        unsigned char b = rgbData[i+2];

        byte >>= 2;
        
        if (r)
        {
            byte |= 0b11000000;
        }
        else  if (g)
        {
            byte |= 0b10000000;
        }
        else  if (b)
        {
            byte |= 0b01000000;
        }
        
        if (++shift > 3)
        {
            headerFile << "0x" << std::hex << std::setw(2) << std::setfill('0') << byte << ", ";
            shift = 0;
            byte = 0;
            
            if (++count >= byte_width)
            {
                headerFile << std::endl << "\t\t";
                count = 0;
            }
        }
    }
    headerFile << std::endl << "\t}," << std::endl;

    std::cout << "Header file written to " << filename << std::endl;
}

int main(int argc, char* argv[]) {
    int width, height;
    std::vector<unsigned char> rgbData;

    if (!readBMP("sprite.bmp", width, height, rgbData)) {
        return 1;
    }

    writeCHeader("sprite.h", width, height, rgbData);

    return 0;
}
