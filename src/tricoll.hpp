// for reading (r1) & writing (r2) TricollBevelIndices
#include <cstdint>


struct BitReader {
    uint64_t  buffer;
    uint64_t  bitsReadFromBuffer;
    uint32_t *fillBuffer;

    BitReader(uint32_t *startBuffer, uint64_t startBit) {
        fillBuffer = &startBuffer[startBit / 32];
        buffer = *fillBuffer++;
        buffer |= (uint64_t)*fillBuffer++ << 32;
        buffer = buffer >> (startBit & 0x1F);
        bitsReadFromBuffer = startBit & 0x1F;
    }

    uint32_t Read10() {
        uint32_t res = buffer & 0x3FF;
        buffer = buffer >> 10;
        bitsReadFromBuffer += 10;
        if (bitsReadFromBuffer >= 0x20) {
            buffer |= (uint64_t)*fillBuffer++ << (64 - bitsReadFromBuffer);
            bitsReadFromBuffer -= 32;
        }
        return res;
    }
};


void write11Bit(uint32_t *writeBuffer, uint64_t offset, uint32_t data) {
    uint64_t intOffset = offset / 32;
    uint32_t bitOffset = offset & 0x1F;
    uint64_t buffer = (uint64_t)writeBuffer[intOffset] | (((uint64_t)writeBuffer[intOffset + 1]) << 32);
    buffer &= ~(0x7FFll << bitOffset);
    buffer |= ((uint64_t)(data & 0x7FF)) << bitOffset;
    writeBuffer[intOffset] = (uint32_t)buffer;
    writeBuffer[intOffset + 1] = buffer >> 32;
}
