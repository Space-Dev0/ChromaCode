
#include "encoder.hpp"

void encode::setDefaultPalette(int color_number, std::vector<rgb> &palette)
{
    if (color_number == 4)
    {
        palette.push_back(black);
        palette.push_back(magenta);
        palette.push_back(yellow);
        palette.push_back(cyan);
    }
    else if (color_number == 8)
    {
        palette.push_back(black);
        palette.push_back(blue);
        palette.push_back(green);
        palette.push_back(cyan);
        palette.push_back(red);
        palette.push_back(magenta);
        palette.push_back(yellow);
        palette.push_back(white);
    }
    else
        std::cout << "Color should be 4 or 8\n";
}

void encode::setDefaultEccLevels(int symbol_number, std::vector<uint8_t> &ecc_levels)
{
    for (int i = 0; i < symbol_number; i++)
        ecc_levels.push_back(0);
}

void encode::swap_byte(uint8_t *a, uint8_t *b)
{
    uint8_t temp = *b;
    *b = *a;
    *a = temp;
}

void encode::swap_int(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void convert_dec_to_bin(int dec, std::string &bin, int start_position, int length)
{
    if (dec < 0)
        dec += 256;
    for (int j = 0; j < length; j++)
    {
        char t = static_cast<char>(dec % 2);
        bin[start_position + length - 1 - j] = t;
        dec /= 2;
    }
}

encode::encode(int color_number, int symbol_number)
{
    if (color_number != 4 && color_number != 8)
        color_number = DEFAULT_COLOR_NUMBER;
    if (symbol_number < 1 || symbol_number > MAX_SYMBOL_NUMBER)
        symbol_number = DEFAULT_SYMBOL_NUMBER;

    colorNumber = color_number;
    symbolNumber = symbol_number;

    setDefaultPalette(colorNumber, palette);

    setDefaultEccLevels(symbolNumber, symbolEccLevels);
}

std::vector<int> encode::analyzeInputData(std::string &input, int *encodedLength)
{
    int encodeSeqLength = ENC_MAX;
    std::vector<char> seq(input.length());
    std::vector<int> currSeqLen((input.length() + 2) * 14);
    std::vector<int> prevMode(((2 * input.length()) + 2) * 14, ENC_MAX / 2);
    std::vector<int> switchMode(28, ENC_MAX / 2);
    std::vector<int> tempSwitchMode(28, ENC_MAX / 2);

    for (int k = 0; k < 7; k++)
    {
        currSeqLen[k] = currSeqLen[k + 7] = ENC_MAX;
        prevMode[k] = prevMode[k + 7] = ENC_MAX;
    }

    currSeqLen[0] = 0;
    bool jpToNxtChar{false}, confirm{false};
    int currSeqCounter = 0;
    bool isShift{false};
    int nbChar = 0;
    int endLoop = input.length();
    int prevModeIndex = 0;
    for (int i = 0; i < endLoop; i++)
    {
        int tmp = input[nbChar];
        int tmp1 = 0;
        if (nbChar + 1 < input.length())
            tmp1 = input[nbChar + 1];
        if (tmp < 0)
            tmp = 256 + tmp;
        if (tmp1 < 0)
            tmp1 = 256 + tmp1;
        currSeqCounter++;
        for (int j = 0; j < ENCODING_MODES; j++)
        {
            if (encodingTable[tmp][j] > -1 && encodingTable[tmp][j] < 64)
                currSeqLen[(i + 1) * 14 + j] = currSeqLen[(i + 1) * 14 + j + 7] = characterSize[j];
            else if ((encodingTable[tmp][j] == -18 && tmp1 == 10) || (encodingTable[tmp][j] < -18 && tmp1 == 32))
            {
                currSeqLen[(i + 1) * 14 + j] = currSeqLen[(i + 1) * 14 + j + 7] = characterSize[j];
                jpToNxtChar = 1;
            }
            else
                currSeqLen[(i + 1) * 14 + j] = currSeqLen[(i + 1) * 14 + j + 7] = ENC_MAX;
        }
        currSeqLen[(i + 1) * 14 + 6] = currSeqLen[(i + 1) * 14 + 13] = characterSize[6];
        isShift = 0;
        for (int j = 0; j < 14; j++)
        {
            int prev = -1;
            int len = currSeqLen[(i + 1) * 14 + j] + currSeqLen[i * 14 + j] + latchShiftTo[j][j];
            prevMode[currSeqCounter * 14 + j] = j;
            for (int k = 0; k < 14; k++)
            {
                if ((len >= currSeqLen[(i + 1) * 14 + j] + currSeqLen[i * 14 + k] + latchShiftTo[k][j] && k < 13) || (k == 13 && prev == j))
                {
                    len = currSeqLen[(i + 1) * 14 + j] + currSeqLen[i * 14 + k] + latchShiftTo[k][j];
                    if (tempSwitchMode[2 * k] == k)
                        prevMode[currSeqCounter * 14 + j] = tempSwitchMode[2 * k + 1];
                    else
                        prevMode[currSeqCounter * 14 + j] = k;
                    if (k == 13 && prev == j)
                        prev = -1;
                }
            }
            currSeqLen[(i + 1) * 14 + j] = len;
            if (j > 6)
            {
                if ((currSeqLen[(i + 1) * 14 + prevMode[currSeqCounter * 14 + j]] > len ||
                     (jpToNxtChar == 1 && currSeqLen[(i + 1) * 14 + prevMode[currSeqCounter * 14 + j]] + characterSize[(prevMode[currSeqCounter * 14 + j]) % 7] > len)) &&
                    j != 13)
                {
                    int index = prevMode[currSeqCounter * 14 + j];
                    int loop = 1;
                    while (index > 6 && currSeqCounter - loop >= 0)
                    {
                        index = prevMode[(currSeqCounter - loop) * 14 + index];
                        loop++;
                    }

                    currSeqLen[(i + 1) * 14 + index] = len;
                    prevMode[(currSeqCounter + 1) * 14 + index] = j;
                    switchMode[2 * index] = index;
                    switchMode[2 * index + 1] = j;
                    isShift = 1;
                    if (jpToNxtChar == 1 && j == 11)
                    {
                        confirm = 1;
                        prevModeIndex = index;
                    }
                }
                else if ((currSeqLen[(i + 1) * 14 + prevMode[currSeqCounter * 14 + j]] > len ||
                          (jpToNxtChar == 1 && currSeqLen[(i + 1) * 14 + prevMode[currSeqCounter * 14 + j]] + characterSize[prevMode[currSeqCounter * 14 + j] % 7] > len)) &&
                         j == 13)
                {
                    currSeqLen[(i + 1) * 14 + prevMode[currSeqCounter * 14 + j]] = len;
                    prevMode[(currSeqCounter + 1) * 14 + prevMode[currSeqCounter * 14 + j]] = j;
                    switchMode[2 * prevMode[currSeqCounter * 14 + j]] = prevMode[currSeqCounter * 14 + j];
                    switchMode[2 * prevMode[currSeqCounter * 14 + j] + 1] = j;
                    isShift = 1;
                }
                if (j != 13)
                    currSeqLen[(i + 1) * 14 + j] = ENC_MAX;
                else
                    prev = prevMode[currSeqCounter * 14 + j];
            }
        }
        for (int j = 0; j < 28; j++)
        {
            tempSwitchMode[j] = switchMode[j];
            switchMode[j] = ENC_MAX / 2;
        }

        if (jpToNxtChar == 1 && confirm == 1)
        {
            for (int j = 0; j <= 2 * ENCODING_MODES + 1; j++)
            {
                if (j != prevModeIndex)
                    currSeqLen[(i + 1) * 14 + j] = ENC_MAX;
            }
            nbChar++;
            endLoop--;
        }
        jpToNxtChar = 0;
        confirm = 0;
        nbChar++;
    }

    int current_mode = 0;
    for (int j = 0; j <= 2 * ENCODING_MODES + 1; j++)
    {
        if (encodeSeqLength > currSeqLen[(nbChar - (input.length() - endLoop)) * 14 + j])
        {
            encodeSeqLength = currSeqLen[(nbChar - (input.length() - endLoop)) * 14 + j];
            current_mode = j;
        }
    }
    if (current_mode > 6)
        isShift = 1;
    if (isShift && tempSwitchMode[2 * current_mode + 1] < 14)
        current_mode = tempSwitchMode[2 * current_mode + 1];

    std::vector<int> encodeSeq(currSeqCounter + 1 + isShift);

    int counter = 0;
    int seq_len = 0;
    int modeswitch = 0;
    encodeSeq[currSeqCounter] = current_mode;
    seq_len += characterSize[encodeSeq[currSeqCounter] % 7];
    for (int i = currSeqCounter; i > 0; i--)
    {
        if (encodeSeq[i] == 13 || encodeSeq[i] == 6)
            counter++;
        else
        {
            if (counter > 15)
            {
                encodeSeqLength += 13;
                seq_len += 13;

                if (counter > 8207)
                {
                    if (encodeSeq[i] == 0 || encodeSeq[i] == 1 || encodeSeq[i] == 7 || encodeSeq[i] == 8)
                        modeswitch = 11;
                    if (encodeSeq[i] == 2 || encodeSeq[i] == 9)
                        modeswitch = 10;
                    if (encodeSeq[i] == 5 || encodeSeq[i] == 12)
                        modeswitch = 12;
                    int remain_in_byte_mode = counter / 8207;
                    int remain_in_byte_mode_residual = counter % 8207;
                    encodeSeqLength += (remain_in_byte_mode)*modeswitch;
                    seq_len += (remain_in_byte_mode)*modeswitch;
                    if (remain_in_byte_mode_residual < 16)
                    {
                        encodeSeqLength += (remain_in_byte_mode - 1) * 13;
                        seq_len += (remain_in_byte_mode - 1) * 13;
                    }
                    else
                    {
                        encodeSeqLength += remain_in_byte_mode * 13;
                        seq_len += remain_in_byte_mode * 13;
                    }
                    if (remain_in_byte_mode_residual == 0)
                    {
                        encodeSeqLength -= modeswitch;
                        seq_len -= modeswitch;
                    }
                }
                counter = 0;
            }
        }
        if (encodeSeq[i] < 14 && i - 1 != 0)
        {
            encodeSeq[i - 1] = prevMode[i * 14 + encodeSeq[i]];
            seq_len += characterSize[encodeSeq[i - 1] % 7];
            if (encodeSeq[i - 1] != encodeSeq[i])
                seq_len += latchShiftTo[encodeSeq[i - 1]][encodeSeq[i]];
        }
        else if (i - 1 == 0)
        {
            encodeSeq[i - 1] = 0;
            if (encodeSeq[i - 1] != encodeSeq[i])
                seq_len += latchShiftTo[encodeSeq[i - 1]][encodeSeq[i]];
            if (counter > 15)
            {
                encodeSeqLength += 13;
                seq_len += 13;

                if (counter > 8207)
                {
                    modeswitch = 11;
                    int remain_in_byte_mode = counter / 8207;
                    int remain_in_byte_mode_residual = counter % 8207;
                    encodeSeqLength += remain_in_byte_mode * modeswitch;
                    seq_len += remain_in_byte_mode * modeswitch;
                    if (remain_in_byte_mode_residual < 16)
                    {
                        encodeSeqLength += (remain_in_byte_mode - 1) * 13;
                        seq_len += (remain_in_byte_mode - 1) * 13;
                    }
                    else
                    {
                        encodeSeqLength += remain_in_byte_mode * 13;
                        seq_len += remain_in_byte_mode * 13;
                    }
                    if (remain_in_byte_mode_residual == 0)
                    {
                        encodeSeqLength -= modeswitch;
                        seq_len -= modeswitch;
                    }
                }
                counter = 0;
            }
        }
        else
            exit(12);
    }
    *encodedLength = encodeSeqLength;
    return encodeSeq;
}

bool isDefaultMode(encode &enc)
{
    if (enc.colorNumber == 8 && (enc.symbolEccLevels[0] == 0 || enc.symbolEccLevels[0] == DEFAULT_ECC_LEVEL))
        return true;
    return false;
}

int getMetadataLength(encode &enc, int index)
{
    int length = 0;

    if (index == 0)
    {
        if (isDefaultMode(enc))
            length = 0;
        else
        {
            length += MASTER_METADATA_PART1_LENGTH;
            length += MASTER_METADATA_PART2_LENGTH;
        }
    }

    else
    {
        length += 2;
        int host_index = enc.symbols[index].host;
        if (enc.symbolVersions[index].x != enc.symbolVersions[host_index].x || enc.symbolVersions[index].y != enc.symbolVersions[host_index].y)
            length += 5;
        if (enc.symbolEccLevels[index] != enc.symbolEccLevels[host_index])
        {
            length += 6;
        }
    }
    return length;
}

int getSymbolCapacity(encode &enc, int index)
{
    int nb_modules_fp;
    if (index == 0)
    {
        nb_modules_fp = 4 * 17;
    }
    else
    {
        nb_modules_fp = 4 * 7;
    }

    int nb_modules_palette = enc.colorNumber > 64 ? (64 - 2) * COLOR_PALETTE_NUMBER : (enc.colorNumber - 2) * COLOR_PALETTE_NUMBER;
    int side_size_x = VERSION2SIZE(enc.symbolVersions[index].x);
    int side_size_y = VERSION2SIZE(enc.symbolVersions[index].y);
    int number_of_aps_x = apNum[enc.symbolVersions[index].x - 1];
    int number_of_aps_y = apNum[enc.symbolVersions[index].y - 1];
    int nb_modules_ap = (number_of_aps_x * number_of_aps_y - 4) * 7;
    int nb_of_bpm = log(enc.colorNumber) / log(2);
    int nb_modules_metadata = 0;
    if (index == 0)
    {
        int nb_metadata_bits = getMetadataLength(enc, index);
        if (nb_metadata_bits > 0)
        {
            nb_modules_metadata = (nb_metadata_bits - MASTER_METADATA_PART1_LENGTH) / nb_of_bpm;
            if ((nb_metadata_bits - MASTER_METADATA_PART1_LENGTH) % nb_of_bpm != 0)
            {
                nb_modules_metadata++;
            }
            nb_modules_metadata += MASTER_METADATA_PART1_MODULE_NUMBER;
        }
    }
    int capacity = (side_size_x * side_size_y - nb_modules_fp - nb_modules_ap - nb_modules_palette - nb_modules_metadata) * nb_of_bpm;
    return capacity;
}

void getOptimalECC(int capacity, int net_data_length, int *wcwr)
{
    float min = capacity;
    for (int k = 3; k <= 6 + 2; k++)
    {
        for (int j = k + 1; j <= 6 + 3; j++)
        {
            int dist = (capacity / j) * j - (capacity / j) * k - net_data_length; // max_gross_payload = floor(capacity / wr) * wr
            if (dist < min && dist >= 0)
            {
                wcwr[1] = j;
                wcwr[0] = k;
                min = dist;
            }
        }
    }
}

std::string encodeData(std::string data, int encodedLength, std::vector<int> &encodeSeq)
{
    std::string encodedData;

    encodedData.resize(encodedLength);

    int counter = 0;
    bool shiftBack = 0;
    int position = 0;
    int currentEncodedLength = 0;
    int endLoop = data.length();
    int byteOffset = 0;
    int byteCounter = 0;
    int factor = 1;
    // encoding starts in upper case mode
    for (int i = 0; i < endLoop; i++)
    {
        int tmp = data[currentEncodedLength];
        if (tmp < 0)
            tmp += 256;
        if (position < encodedLength)
        {
            int decimal_value;
            // check if mode is switched
            if (encodeSeq[counter] != encodeSeq[counter + 1])
            {
                // encode mode switch
                int length = latchShiftTo[encodeSeq[counter]][encodeSeq[counter + 1]];
                if (encodeSeq[counter + 1] == 6 || encodeSeq[counter + 1] == 13)
                    length -= 4;
                if (length < ENC_MAX)
                    convert_dec_to_bin(modeSwitch[encodeSeq[counter]][encodeSeq[counter + 1]], encodedData, position, length);
                else
                {
                    exit(13);
                }
                position += latchShiftTo[encodeSeq[counter]][encodeSeq[counter + 1]];
                if (encodeSeq[counter + 1] == 6 || encodeSeq[counter + 1] == 13)
                    position -= 4;
                // check if latch or shift
                if ((encodeSeq[counter + 1] > 6 && encodeSeq[counter + 1] <= 13) || (encodeSeq[counter + 1] == 13 && encodeSeq[counter + 2] != 13))
                    shiftBack = 1; // remember to shift back to mode from which was invoked
            }
            // if not byte mode
            if (encodeSeq[counter + 1] % 7 != 6) // || endLoop-1 == i)
            {
                if (encodingTable[tmp][encodeSeq[counter + 1] % 7] > -1 && characterSize[encodeSeq[counter + 1] % 7] < ENC_MAX)
                {
                    // encode character
                    convert_dec_to_bin(encodingTable[tmp][encodeSeq[counter + 1] % 7], encodedData, position, characterSize[encodeSeq[counter + 1] % 7]);
                    position += characterSize[encodeSeq[counter + 1] % 7];
                    counter++;
                }
                else if (encodingTable[tmp][encodeSeq[counter + 1] % 7] < -1)
                {
                    int tmp1 = data[currentEncodedLength + 1];
                    if (tmp1 < 0)
                        tmp1 += 256;
                    // read next character to see if more efficient encoding possible
                    if (((tmp == 44 || tmp == 46 || tmp == 58) && tmp1 == 32) || (tmp == 13 && tmp1 == 10))
                        decimal_value = abs(encodingTable[tmp][encodeSeq[counter + 1] % 7]);
                    else if (tmp == 13 && tmp1 != 10)
                        decimal_value = 18;
                    else
                    {
                        exit(13);
                    }
                    if (characterSize[encodeSeq[counter + 1] % 7] < ENC_MAX)
                        convert_dec_to_bin(decimal_value, encodedData, position, characterSize[encodeSeq[counter + 1] % 7]);
                    position += characterSize[encodeSeq[counter + 1] % 7];
                    counter++;
                    endLoop--;
                    currentEncodedLength++;
                }
                else
                {
                    exit(13);
                }
            }
            else
            {
                if (encodeSeq[counter] != encodeSeq[counter + 1])
                {
                    byteCounter = 0;
                    for (int byte_loop = counter + 1; byte_loop <= endLoop; byte_loop++)
                    {
                        if (encodeSeq[byte_loop] == 6 || encodeSeq[byte_loop] == 13)
                            byteCounter++;
                        else
                            break;
                    }
                    convert_dec_to_bin(byteCounter > 15 ? 0 : byteCounter, encodedData, position, 4);
                    position += 4;
                    if (byteCounter > 15)
                    {
                        if (byteCounter <= 8207) // 8207=2^13+15; if number of bytes exceeds 8207, encoder shall shift to byte mode again from upper case mode && byteCounter < 8207
                        {
                            convert_dec_to_bin(byteCounter - 15 - 1, encodedData, position, 13);
                        }
                        else
                        {
                            convert_dec_to_bin(8191, encodedData, position, 13);
                        }
                        position += 13;
                    }
                    byteOffset = byteCounter;
                }
                if (byteOffset - byteCounter == factor * 8207) // byte mode exceeds 2^13 + 15
                {
                    if (encodeSeq[counter - (byteOffset - byteCounter)] == 0 || encodeSeq[counter - (byteOffset - byteCounter)] == 7 || encodeSeq[counter - (byteOffset - byteCounter)] == 1 || encodeSeq[counter - (byteOffset - byteCounter)] == 8)
                    {
                        convert_dec_to_bin(124, encodedData, position, 7); // shift from upper case to byte
                        position += 7;
                    }
                    if (encodeSeq[counter - (byteOffset - byteCounter)] == 2 || encodeSeq[counter - (byteOffset - byteCounter)] == 9)
                    {
                        convert_dec_to_bin(60, encodedData, position, 5); // shift from numeric to byte
                        position += 5;
                    }
                    if (encodeSeq[counter - (byteOffset - byteCounter)] == 5 || encodeSeq[counter - (byteOffset - byteCounter)] == 12)
                    {
                        convert_dec_to_bin(252, encodedData, position, 8); // shift from alphanumeric to byte
                        position += 8;
                    }
                    convert_dec_to_bin(byteCounter > 15 ? 0 : byteCounter, encodedData, position, 4); // write the first 4 bits
                    position += 4;
                    if (byteCounter > 15) // if more than 15 bytes -> use the next 13 bits to wirte the length
                    {
                        if (byteCounter <= 8207) // 8207=2^13+15; if number of bytes exceeds 8207, encoder shall shift to byte mode again from upper case mode && byteCounter < 8207
                        {
                            convert_dec_to_bin(byteCounter - 15 - 1, encodedData, position, 13);
                        }
                        else // number exceeds 2^13 + 15
                        {
                            convert_dec_to_bin(8191, encodedData, position, 13);
                        }
                        position += 13;
                    }
                    factor++;
                }
                if (characterSize[encodeSeq[counter + 1] % 7] < ENC_MAX)
                    convert_dec_to_bin(tmp, encodedData, position, characterSize[encodeSeq[counter + 1] % 7]);
                else
                {
                    exit(13);
                }
                position += characterSize[encodeSeq[counter + 1] % 7];
                counter++;
                byteCounter--;
            }
            // shift back to mode from which mode was invoked
            if (shiftBack && byteCounter == 0)
            {
                if (byteOffset == 0)
                    encodeSeq[counter] = encodeSeq[counter - 1];
                else
                    encodeSeq[counter] = encodeSeq[counter - byteOffset];
                shiftBack = 0;
                byteOffset = 0;
            }
        }
        else
        {
            exit(13);
        }
        currentEncodedLength++;
    }
    return encodedData;
}

int main()
{
}
