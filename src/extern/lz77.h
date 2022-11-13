#pragma once
#include <stdint.h>

uint32_t lz77_decompress(uint8_t *input, uint32_t input_length, uint8_t *output, uint32_t output_limit, char match_len_bits) {
    const uint16_t WindowSize = (1 << match_len_bits) - 1;
    const uint16_t MatchMaxLenght = (1 << match_len_bits) - 1; // also used as bitmask for extracting length from match

    uint32_t output_cursor = 0;
    uint8_t input_literal_bitmask = input[0];
    uint8_t input_bitmask = 0;

    for (uint32_t input_cursor = 0; input_cursor < input_length;) {
        if (input_bitmask == 0) {
            input_bitmask = 1;
            input_literal_bitmask = input[input_cursor++];
        }

        if (input_literal_bitmask & input_bitmask) {
            // literal match
            if (input_cursor < input_length && output_cursor < output_limit) {
                output[output_cursor++] = input[input_cursor++];
            }
            else {
                output_cursor++;
                input_cursor++;
            }
        }
        else {
            // window match
            if (input_cursor < input_length - 1) {
                uint8_t h = input[input_cursor++];
                uint8_t l = input[input_cursor++];

                uint16_t match = (h << 8) | l;
                uint16_t length = match & MatchMaxLenght;

                uint16_t offset = match >> match_len_bits;
                for (uint16_t i = 0; i < length; i++) {
                    if (output_cursor < output_limit) {
                        output[output_cursor] = output[output_cursor - offset];
                    }
                    output_cursor++;
                }
            }
            else {
                input_cursor += 2;
            }
        }

        input_bitmask <<= 1;
    }
    return output_cursor;
}