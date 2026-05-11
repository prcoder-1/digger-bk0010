#include <iostream>
#include <fstream>
#include <cstdint>

// Include the GIMP header file here
#include "digger_sprites.c"

// BMP file header (14 bytes)
#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType = 0x4D42; // "BM"
    uint32_t bfSize;
    uint16_t bfReserved1 = 0;
    uint16_t bfReserved2 = 0;
    uint32_t bfOffBits = 54;  // Pixel data offset
};

// BMP info header (40 bytes)
struct BMPInfoHeader {
    uint32_t biSize = 40;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 24; // 24-bit bitmap
    uint32_t biCompression = 0;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter = 0;
    int32_t  biYPelsPerMeter = 0;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
};
#pragma pack(pop)

void writeBMP(const char* filename, int width, int height, const unsigned char *data) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Cannot open output file\n";
        return;
    }

    int rowSize = (3 * width + 3) & (~3); // Row size in bytes (padded to 4 byte boundary)
    int dataSize = rowSize * height;

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biSizeImage = dataSize;

    fileHeader.bfSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + dataSize;

    outFile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    outFile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    // BMP stores pixels bottom-up, so write rows in reverse order
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            int pix_no = y * width + x;
            int byte_no = pix_no / 4; // 4 pixels per byte
            int offset = pix_no % 4;
            
            unsigned char color = (data[byte_no] >> (offset * 2)) & 3;
            
            unsigned char g = 0;
            unsigned char b = 0;
            unsigned char r = 0;
            
            switch (color)
            {
                case 0:
                {
                    break;
                }
                
                case 1:
                {
                    b = 255;
                    break;
                }
                
                case 2:
                {
                    g = 255;
                    break;
                }
                
                case 3:
                {
                    r = 255;
                    break;
                }
            }
            
            // Write pixels in BGR format
            outFile.put(b);
            outFile.put(g);
            outFile.put(r);
        }
        // Padding for 4-byte alignment
        for (int p = 0; p < (rowSize - 3 * width); ++p) {
            outFile.put(0);
        }
    }

    outFile.close();
    std::cout << "BMP image written to " << filename << "\n";
}

int main() {
    writeBMP("sprite.bmp", 16, 15, (unsigned char *)image_hobbin_right[2]);
    return 0;
}
