
// p4fliconv: high resolution interlaced FLI converter utility
// Copyright (C) 2007-2017 Istvan Varga <istvanv@users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// The Plus/4 program files generated by this utility are not covered by the
// GNU General Public License, and can be used, modified, and distributed
// without any restrictions.

#include "p4fliconv.hpp"
#include "prgdata.hpp"
#include "p4slib.hpp"

#include <vector>
#include "pngwrite.hpp"
#include "decompress5.hpp"

namespace Plus4FLIConv {

  void writeP4SFile(const char *fileName, PRGData& prgData)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Plus4Emu::Exception("invalid file name");
    std::vector< unsigned char >  outBuf;
    // write file header
    outBuf.push_back((unsigned char) 0x50);     // 'P'
    outBuf.push_back((unsigned char) 0x34);     // '4'
    outBuf.push_back((unsigned char) 0x53);     // 'S'
    outBuf.push_back((unsigned char) 0x3A);     // ':'
    int     imageWidth = 320;
    int     imageHeight = 200;
    int     imageFlags = 5;
    int     color0 = 0x00;
    int     color3 = 0x00;
    std::vector< unsigned char >  buf_ColoR;
    std::vector< unsigned char >  buf_BrighT;
    std::vector< unsigned char >  buf_PixeL;
    std::vector< unsigned char >  buf_FLI_ColoR;
    std::vector< unsigned char >  buf_FLI_BrighT;
    std::vector< unsigned char >  buf_FLI_FF15;
    std::vector< unsigned char >  buf_FLI_FF16;
    if (prgData.getConversionType() >= 6) {
      if (prgData.getConversionType() == 8) {
        // check for Logo Editor format (128x64 multicolor character set)
        imageWidth = 256;
        imageHeight = 64;
        color0 = int(prgData[0x1100 - 0x0FFF]);
        int     color1 = int(prgData[0x1101 - 0x0FFF]);
        int     color2 = int(prgData[0x1102 - 0x0FFF]);
        color3 = int(prgData[0x1103 - 0x0FFF]);
        if (color0 == 0 && color1 == 0 && color2 == 0 && color3 == 0) {
          // if no palette is available, use a default greyscale palette
          color0 = 0x00;
          color1 = 0x31;
          color2 = 0x51;
          color3 = 0x71;
        }
        imageFlags ^= 3;
        buf_ColoR.resize(120 * 8);
        buf_BrighT.resize(120 * 8);
        buf_PixeL.resize(960 * 64);
        int     l = (color2 & 0x70) | ((color1 & 0x70) >> 4);
        int     c = (color2 & 0x0F) | ((color1 & 0x0F) << 4);
        // store attributes
        for (int yc = 0; yc < 8; yc++) {
          for (int xc = 0; xc < 32; xc++) {
            buf_ColoR[(yc * 120) + xc] = (unsigned char) c;
            buf_BrighT[(yc * 120) + xc] = (unsigned char) l;
          }
        }
        // store bitmap data
        for (int yc = 0; yc < 64; yc++) {
          for (int xc = 0; xc < 256; xc++) {
            buf_PixeL[(yc * 960) + xc] =
                (unsigned char) (bool(prgData[((xc >> 3) * 64) + yc
                                              + (0x6000 - 0x0FFF)]
                                      & (unsigned char) (1 << (7 - (xc & 7)))));
          }
        }
      }
      else {
        buf_ColoR.resize(120 * 25);
        buf_BrighT.resize(120 * 25);
        buf_PixeL.resize(960 * 200);
        // check for Multi Botticelli format
        if (prgData.getConversionType() == 7) {
          imageFlags ^= 3;
          color0 = int(prgData[0x7BFF - 0x0FFF]);
          color3 = int(prgData[0x7BFE - 0x0FFF]);
          color0 = ((color0 & 0x07) << 4) | ((color0 & 0xF0) >> 4);
          color3 = ((color3 & 0x07) << 4) | ((color3 & 0xF0) >> 4);
        }
        // convert attributes
        for (int yc = 0; yc < 25; yc++) {
          for (int xc = 0; xc < 40; xc++) {
            unsigned char l = prgData[(yc * 40) + xc + (0x7800 - 0x0FFF)];
            unsigned char c = prgData[(yc * 40) + xc + (0x7C00 - 0x0FFF)];
            if ((imageFlags & 1) != 0) {
              // need to swap colors in hires mode
              l = ((l & 0x07) << 4) | ((l & 0x70) >> 4);
              c = ((c & 0x0F) << 4) | ((c & 0xF0) >> 4);
            }
            buf_BrighT[(yc * 120) + xc] = l & 0x77;
            buf_ColoR[(yc * 120) + xc] = c;
          }
        }
        // convert bitmap data
        for (int yc = 0; yc < 200; yc++) {
          for (int xc = 0; xc < 320; xc++) {
            buf_PixeL[(yc * 960) + xc] =
                (unsigned char) (bool(prgData[((yc >> 3) * 320)
                                              + ((xc >> 3) * 8) + (yc & 7)
                                              + (0x8000 - 0x0FFF)]
                                      & (unsigned char) (1 << (7 - (xc & 7)))));
          }
        }
      }
    }
    else {
      // FLI modes
      imageHeight = prgData.getVerticalSize();
      if (imageHeight >= 256 || (prgData.interlaceFlags() & 0xC0) != 0)
        throw Plus4Emu::Exception("cannot save interlaced FLI in P4S format");
      if (imageHeight < 128 || imageHeight > 248 || (imageHeight & 3) != 0)
        throw Plus4Emu::Exception("invalid image height");
      imageFlags = 8;
      for (int yc = 0; yc < imageHeight; yc++) {
        if ((prgData.lineXShift(yc << 1) & 0x07) != 0) {
          throw Plus4Emu::Exception("cannot save FLI with non-zero X shift "
                                    "in P4S format");
        }
        if ((prgData.lineXShift(yc << 1) & 0x10) != 0)
          imageFlags |= 2;      // multicolor
        else
          imageFlags |= 1;      // hires
      }
      if ((imageFlags & 3) == 3)
        throw Plus4Emu::Exception("cannot save FLI with mixed hires/multicolor "
                                  "mode in P4S format");
      if ((imageFlags & 2) != 0) {
        // multicolor mode: save background colors
        for (int yc = 0; yc < imageHeight; yc++) {
          buf_FLI_FF15.push_back(prgData.lineColor0(yc << 1));
          buf_FLI_FF16.push_back(prgData.lineColor3(yc << 1));
        }
      }
      buf_ColoR.resize(size_t((imageHeight / 8) * 120));
      buf_BrighT.resize(size_t((imageHeight / 8) * 120));
      buf_PixeL.resize(size_t(imageHeight * 960));
      buf_FLI_ColoR.resize(27000);
      buf_FLI_BrighT.resize(27000);
      // convert attributes
      for (int yc = 0; yc < imageHeight; yc += 2) {
        for (int xc = 0; xc < 320; xc += 8) {
          int     l0 = prgData.l0(xc, yc << 1);
          int     c0 = prgData.c0(xc, yc << 1);
          int     l1 = prgData.l1(xc, yc << 1);
          int     c1 = prgData.c1(xc, yc << 1);
          if (l0 != 0)
            l0--;
          else
            c0 = 0;
          if (l1 != 0)
            l1--;
          else
            c1 = 0;
          int     l = (l0 << 4) | l1;
          int     c = c0 | (c1 << 4);
          if ((imageFlags & 1) != 0) {
            // need to swap colors in hires mode
            l = l0 | (l1 << 4);
            c = (c0 << 4) | c1;
          }
          switch (yc & 6) {
          case 0:
            buf_BrighT[((yc >> 3) * 120) + (xc >> 3)] = l;
            buf_ColoR[((yc >> 3) * 120) + (xc >> 3)] = c;
            break;
          case 2:
            buf_FLI_BrighT[((yc >> 3) * 120) + (xc >> 3)] = l;
            buf_FLI_ColoR[((yc >> 3) * 120) + (xc >> 3)] = c;
            break;
          case 4:
            buf_FLI_BrighT[((yc >> 3) * 120) + (xc >> 3) + 9000] = l;
            buf_FLI_ColoR[((yc >> 3) * 120) + (xc >> 3) + 9000] = c;
            break;
          case 6:
            buf_FLI_BrighT[((yc >> 3) * 120) + (xc >> 3) + 18000] = l;
            buf_FLI_ColoR[((yc >> 3) * 120) + (xc >> 3) + 18000] = c;
            break;
          }
        }
      }
      // convert bitmap data
      for (int yc = 0; yc < imageHeight; yc++) {
        for (int xc = 0; xc < 320; xc++) {
          buf_PixeL[(yc * 960) + xc] =
              (unsigned char) prgData.getPixel(xc, yc << 1);
        }
      }
    }
    // write image parameters
    outBuf.push_back((unsigned char) (imageWidth & 0xF8));
    outBuf.push_back((unsigned char) (imageWidth >> 8));
    outBuf.push_back((unsigned char) (imageHeight & 0xF8));
    outBuf.push_back((unsigned char) (imageHeight >> 8));
    outBuf.push_back((unsigned char) imageFlags);
    outBuf.push_back((unsigned char) (color0 & 0x7F));
    outBuf.push_back((unsigned char) (color3 & 0x7F));
    // write all data blocks
    for (int i = 0; i < 7; i++) {
      std::vector< unsigned char >  *bufPtr =
          (std::vector< unsigned char > *) 0;
      const char  *blockName = (char *) 0;
      switch (i) {
      case 0:
        bufPtr = &buf_ColoR;
        blockName = "<ColoR>";
        break;
      case 1:
        bufPtr = &buf_BrighT;
        blockName = "<BrighT>";
        break;
      case 2:
        bufPtr = &buf_PixeL;
        blockName = "<PixeL>";
        break;
      case 3:
        bufPtr = &buf_FLI_ColoR;
        blockName = "<FLI_ColoR>";
        break;
      case 4:
        bufPtr = &buf_FLI_BrighT;
        blockName = "<FLI_BrighT>";
        break;
      case 5:
        bufPtr = &buf_FLI_FF15;
        blockName = "<FLI_FF15>";
        break;
      case 6:
        bufPtr = &buf_FLI_FF16;
        blockName = "<FLI_FF16>";
        break;
      }
      if (bufPtr->size() < 1)
        continue;
      // compress data
      std::vector< unsigned char >  compressBuf;
      {
        Plus4Emu::Compressor_ZLib *compressor = (Plus4Emu::Compressor_ZLib *) 0;
        compressor = new Plus4Emu::Compressor_ZLib();
        try {
          compressor->compressData(compressBuf,
                                   &(bufPtr->front()), bufPtr->size(), 32768);
        }
        catch (...) {
          delete compressor;
          throw;
        }
        delete compressor;
      }
      // write compressed data to output buffer
      outBuf.push_back((unsigned char) (bufPtr->size() & 0xFF));
      outBuf.push_back((unsigned char) ((bufPtr->size() >> 8) & 0xFF));
      outBuf.push_back((unsigned char) ((bufPtr->size() >> 16) & 0xFF));
      outBuf.push_back((unsigned char) ((bufPtr->size() >> 24) & 0xFF));
      for (size_t j = 0; true; j++) {
        outBuf.push_back((unsigned char) blockName[j]);
        if (blockName[j] == '\0')
          break;
      }
      for (size_t j = 0; j < compressBuf.size(); j++)
        outBuf.push_back(compressBuf[j]);
    }
    // write output file
    std::FILE *f = Plus4Emu::fileOpen(fileName, "wb");
    if (!f)
      throw Plus4Emu::Exception("error opening P4S file");
    for (size_t i = 0; i < outBuf.size(); i++) {
      if (std::fputc(int(outBuf[i]), f) == EOF) {
        std::fclose(f);
        throw Plus4Emu::Exception("error writing P4S file");
      }
    }
    if (std::fflush(f) != 0) {
      std::fclose(f);
      throw Plus4Emu::Exception("error writing P4S file");
    }
    std::fclose(f);
  }

  bool readP4SFile(const char *fileName, PRGData& prgData,
                   unsigned int& prgStartAddr, unsigned int& prgEndAddr)
  {
    prgData.setConversionType(6);
    prgData.clear();
    prgStartAddr = 0x1001U;
    prgEndAddr = 0x1003U;
    if (fileName == (char *) 0 || fileName[0] == '\0')
      throw Plus4Emu::Exception("invalid file name");
    std::FILE *f = Plus4Emu::fileOpen(fileName, "rb");
    if (!f)
      throw Plus4Emu::Exception("error opening file");
    try {
      std::vector< unsigned char >  inBuf;
      // read file into memory, and check file type
      while (true) {
        int     c = std::fgetc(f);
        if (c == EOF)
          break;
        if (inBuf.size() >= 1048576) {
          std::fclose(f);
          return false;
        }
        inBuf.push_back((unsigned char) (c & 0xFF));
        if (inBuf.size() == 4) {
          if (!(inBuf[0] == 0x50 && inBuf[1] == 0x34 &&         // "P4S:"
                inBuf[2] == 0x53 && inBuf[3] == 0x3A)) {
            std::fclose(f);
            return false;
          }
        }
      }
      std::fclose(f);
      f = (std::FILE *) 0;
      if (inBuf.size() < 12)
        return false;
      // read and check image parameters
      int     imageWidth = (int(inBuf[4]) & 0xF8) | (int(inBuf[5]) << 8);
      int     imageHeight = (int(inBuf[6]) & 0xF8) | (int(inBuf[7]) << 8);
      int     imageFlags = int(inBuf[8]);
      int     color0 = int(inBuf[9]) & 0x7F;
      int     color3 = int(inBuf[10]) & 0x7F;
      if (imageWidth != 320 || imageHeight < 128 || imageHeight > 248)
        throw Plus4Emu::Exception("unsupported P4S image size");
      if (imageFlags & (~(int(15))))
        throw Plus4Emu::Exception("unsupported P4S image flags");
      if (!((imageFlags & 15) == 5 || (imageFlags & 15) == 6 ||
            (imageFlags & 15) == 9 || (imageFlags & 15) == 10)) {
        throw Plus4Emu::Exception("invalid P4S image flags");
      }
      if ((imageFlags & 20) == 20)
        throw Plus4Emu::Exception("invalid P4S image flags");
      // read all P4S data blocks
      std::vector< unsigned char >  buf_ColoR;
      std::vector< unsigned char >  buf_BrighT;
      std::vector< unsigned char >  buf_PixeL;
      std::vector< unsigned char >  buf_FLI_ColoR;
      std::vector< unsigned char >  buf_FLI_BrighT;
      std::vector< unsigned char >  buf_FLI_FF15;
      std::vector< unsigned char >  buf_FLI_FF16;
      std::vector< unsigned char >  buf_dummy;
      size_t  readPos = 11;
      while (readPos < inBuf.size()) {
        if ((readPos + 4) > inBuf.size())
          throw Plus4Emu::Exception("unexpected end of P4S image data");
        size_t  uncompressedSize = size_t(inBuf[readPos])
                                   | (size_t(inBuf[readPos + 1]) << 8)
                                   | (size_t(inBuf[readPos + 2]) << 16)
                                   | (size_t(inBuf[readPos + 3]) << 24);
        readPos = readPos + 4;
        if (uncompressedSize < 1 || uncompressedSize > 1048576)
          throw Plus4Emu::Exception("error in P4S image data");
        char    blockName[16];
        for (int i = 0; i < 16; i++) {
          if (readPos >= inBuf.size())
            throw Plus4Emu::Exception("unexpected end of P4S image data");
          blockName[i] = char(inBuf[readPos]);
          readPos++;
          if (blockName[i] == '\0')
            break;
          if (i == 15)
            throw Plus4Emu::Exception("error in P4S image data");
        }
        if (readPos >= inBuf.size())
          throw Plus4Emu::Exception("unexpected end of P4S image data");
        std::vector< unsigned char >  *bufPtr =
            (std::vector< unsigned char > *) 0;
        if (std::strcmp(&(blockName[0]), "<ColoR>") == 0)
          bufPtr = &buf_ColoR;
        if (std::strcmp(&(blockName[0]), "<BrighT>") == 0)
          bufPtr = &buf_BrighT;
        if (std::strcmp(&(blockName[0]), "<PixeL>") == 0)
          bufPtr = &buf_PixeL;
        if (std::strcmp(&(blockName[0]), "<FLI_ColoR>") == 0)
          bufPtr = &buf_FLI_ColoR;
        if (std::strcmp(&(blockName[0]), "<FLI_BrighT>") == 0)
          bufPtr = &buf_FLI_BrighT;
        if (std::strcmp(&(blockName[0]), "<FLI_FF15>") == 0)
          bufPtr = &buf_FLI_FF15;
        if (std::strcmp(&(blockName[0]), "<FLI_FF16>") == 0)
          bufPtr = &buf_FLI_FF16;
        if (std::strcmp(&(blockName[0]), "<IFLI_ColoR>") == 0)
          bufPtr = &buf_dummy;
        if (std::strcmp(&(blockName[0]), "<IFLI_BrighT>") == 0)
          bufPtr = &buf_dummy;
        if (std::strcmp(&(blockName[0]), "<IFLI_PixeL>") == 0)
          bufPtr = &buf_dummy;
        if (std::strcmp(&(blockName[0]), "<IFLI_FF15>") == 0)
          bufPtr = &buf_dummy;
        if (std::strcmp(&(blockName[0]), "<IFLI_FF16>") == 0)
          bufPtr = &buf_dummy;
        if (std::strcmp(&(blockName[0]), "<PrevieW>") == 0)
          bufPtr = &buf_dummy;
        if (!bufPtr)
          throw Plus4Emu::Exception("error in P4S image data");
        bufPtr->clear();
        // decompress data
        Plus4Compress::Decompressor_ZLib  *decompressor =
            (Plus4Compress::Decompressor_ZLib *) 0;
        decompressor = new Plus4Compress::Decompressor_ZLib();
        try {
          decompressor->decompressData(*bufPtr, inBuf, &readPos);
        }
        catch (...) {
          delete decompressor;
          throw;
        }
        delete decompressor;
        if (bufPtr->size() != uncompressedSize)
          throw Plus4Emu::Exception("error reading compressed data");
      }
      if ((imageFlags & 4) != 0 && imageHeight == 200) {
        // simple non-FLI formats (hires and multicolor)
        if (buf_ColoR.size() < 3000 || buf_BrighT.size() < 3000 ||
            buf_PixeL.size() < 192000) {
          throw Plus4Emu::Exception("error in P4S image data");
        }
        prgData.setConversionType((imageFlags & 2) == 0 ? 6 : 7);
        prgData.clear();
        // convert attributes
        for (int yc = 0; yc < 25; yc++) {
          for (int xc = 0; xc < 40; xc++) {
            prgData[(yc * 40) + xc + (0x7800 - 0x0FFF)] =
                buf_BrighT[(yc * 120) + xc] & 0x77;
            prgData[(yc * 40) + xc + (0x7C00 - 0x0FFF)] =
                buf_ColoR[(yc * 120) + xc];
          }
        }
        if ((imageFlags & 2) != 0) {
          // multicolor mode
          prgData[0x7BFA - 0x0FFF] = 0x4D;      // 'M'
          prgData[0x7BFB - 0x0FFF] = 0x55;      // 'U'
          prgData[0x7BFC - 0x0FFF] = 0x4C;      // 'L'
          prgData[0x7BFD - 0x0FFF] = 0x54;      // 'T'
          prgData[0x7BFE - 0x0FFF] =
              (unsigned char) (((color3 & 0x70) >> 4) | ((color3 & 0x0F) << 4));
          prgData[0x7BFF - 0x0FFF] =
              (unsigned char) (((color0 & 0x70) >> 4) | ((color0 & 0x0F) << 4));
        }
        // convert bitmap data
        for (int yc = 0; yc < 200; yc++) {
          for (int xc = 0; xc < 320; xc++) {
            bool    b = bool(buf_PixeL[(yc * 960) + xc]);
            prgData[((yc >> 3) * 320) + ((xc >> 3) * 8) + (yc & 7)
                    + (0x8000 - 0x0FFF)] |=
                (unsigned char) (int(b) << (7 - (xc & 7)));
          }
        }
        if ((imageFlags & 1) != 0) {
          // need to swap colors in hires mode
          for (int i = 0x7800; i < 0x7FE8; i++) {
            if (i == 0x7BE8)
              i = 0x7C00;
            unsigned char&  tmp = prgData[i - 0x0FFF];
            tmp = ((tmp & 0x0F) << 4) | ((tmp & 0xF0) >> 4);
          }
        }
      }
      if ((imageFlags & 4) != 0 && imageHeight != 200) {
        // non-FLI format, but not 200 lines, so need to convert to FLI
        if (buf_ColoR.size() < size_t((imageHeight / 8) * 120) ||
            buf_BrighT.size() < size_t((imageHeight / 8) * 120) ||
            buf_PixeL.size() < size_t(imageHeight * 960)) {
          throw Plus4Emu::Exception("error in P4S image data");
        }
        prgData.setConversionType((imageFlags & 2) == 0 ? 4 : 5);
        prgData.clear();
        prgData.setVerticalSize(imageHeight);
        prgData.borderColor() = 0x00;
        prgData.lineBlankFXEnabled() = 0x01;
        // convert attributes
        for (int yc = 0; yc < imageHeight; yc += 2) {
          for (int xc = 0; xc < 320; xc += 8) {
            int     l = buf_BrighT[((yc >> 3) * 120) + (xc >> 3)];
            int     c = buf_ColoR[((yc >> 3) * 120) + (xc >> 3)];
            if ((imageFlags & 1) != 0) {
              // need to swap colors in hires mode
              l = ((l & 0x07) << 4) | ((l & 0x70) >> 4);
              c = ((c & 0x0F) << 4) | ((c & 0xF0) >> 4);
            }
            int     l0 = (l & 0x70) >> 4;
            int     c0 = c & 0x0F;
            l0 = (c0 != 0 ? (l0 + 1) : 0);
            int     l1 = l & 0x07;
            int     c1 = (c & 0xF0) >> 4;
            l1 = (c1 != 0 ? (l1 + 1) : 0);
            prgData.l0(xc, yc << 1) = l0;
            prgData.c0(xc, yc << 1) = c0;
            prgData.l1(xc, yc << 1) = l1;
            prgData.c1(xc, yc << 1) = c1;
          }
        }
        if ((imageFlags & 2) != 0) {
          // multicolor mode
          for (int yc = 0; yc < imageHeight; yc++) {
            prgData.lineXShift(yc << 1) = 0x10;
            prgData.lineColor0(yc << 1) = (unsigned char) color0;
            prgData.lineColor3(yc << 1) = (unsigned char) color3;
          }
        }
        // convert bitmaps
        for (int yc = 0; yc < imageHeight; yc++) {
          for (int xc = 0; xc < 320; xc++)
            prgData.setPixel(xc, yc << 1, bool(buf_PixeL[(yc * 960) + xc]));
        }
        prgData.convertImageData();
      }
      if ((imageFlags & 24) == 8) {
        // non-interlaced FLI
        if (buf_ColoR.size() < size_t((imageHeight / 8) * 120) ||
            buf_BrighT.size() < size_t((imageHeight / 8) * 120) ||
            buf_PixeL.size() < size_t(imageHeight * 960) ||
            buf_FLI_ColoR.size() < size_t(27000) ||
            buf_FLI_BrighT.size() < size_t(27000) ||
            ((imageFlags & 2) != 0 &&
             (buf_FLI_FF15.size() < size_t(imageHeight) ||
              buf_FLI_FF16.size() < size_t(imageHeight)))) {
          throw Plus4Emu::Exception("error in P4S image data");
        }
        prgData.setConversionType((imageFlags & 2) == 0 ? 4 : 5);
        prgData.clear();
        prgData.setVerticalSize(imageHeight);
        prgData.borderColor() = 0x00;
        prgData.lineBlankFXEnabled() = 0x01;
        // convert attributes
        for (int yc = 0; yc < imageHeight; yc += 2) {
          for (int xc = 0; xc < 320; xc += 8) {
            int     l = 0;
            int     c = 0;
            switch (yc & 6) {
            case 0:
              l = buf_BrighT[((yc >> 3) * 120) + (xc >> 3)];
              c = buf_ColoR[((yc >> 3) * 120) + (xc >> 3)];
              break;
            case 2:
              l = buf_FLI_BrighT[((yc >> 3) * 120) + (xc >> 3)];
              c = buf_FLI_ColoR[((yc >> 3) * 120) + (xc >> 3)];
              break;
            case 4:
              l = buf_FLI_BrighT[((yc >> 3) * 120) + (xc >> 3) + 9000];
              c = buf_FLI_ColoR[((yc >> 3) * 120) + (xc >> 3) + 9000];
              break;
            case 6:
              l = buf_FLI_BrighT[((yc >> 3) * 120) + (xc >> 3) + 18000];
              c = buf_FLI_ColoR[((yc >> 3) * 120) + (xc >> 3) + 18000];
              break;
            }
            if ((imageFlags & 1) != 0) {
              // need to swap colors in hires mode
              l = ((l & 0x07) << 4) | ((l & 0x70) >> 4);
              c = ((c & 0x0F) << 4) | ((c & 0xF0) >> 4);
            }
            int     l0 = (l & 0x70) >> 4;
            int     c0 = c & 0x0F;
            l0 = (c0 != 0 ? (l0 + 1) : 0);
            int     l1 = l & 0x07;
            int     c1 = (c & 0xF0) >> 4;
            l1 = (c1 != 0 ? (l1 + 1) : 0);
            prgData.l0(xc, yc << 1) = l0;
            prgData.c0(xc, yc << 1) = c0;
            prgData.l1(xc, yc << 1) = l1;
            prgData.c1(xc, yc << 1) = c1;
          }
        }
        if ((imageFlags & 2) != 0) {
          // multicolor mode
          for (int yc = 0; yc < imageHeight; yc++) {
            prgData.lineXShift(yc << 1) = 0x10;
            prgData.lineColor0(yc << 1) = buf_FLI_FF15[yc];
            prgData.lineColor3(yc << 1) = buf_FLI_FF16[yc];
          }
        }
        // convert bitmaps
        for (int yc = 0; yc < imageHeight; yc++) {
          for (int xc = 0; xc < 320; xc++)
            prgData.setPixel(xc, yc << 1, bool(buf_PixeL[(yc * 960) + xc]));
        }
        prgData.convertImageData();
      }
      prgStartAddr = prgData.getImageDataStartAddress();
      prgEndAddr = prgData.getImageDataEndAddress();
    }
    catch (...) {
      if (f)
        std::fclose(f);
      throw;
    }
    return true;
  }

}       // namespace Plus4FLIConv

