
#include <string.h>
#include "encoder.hpp"

void encode::setDefaultPalette(int colorNumber, std::vector<rgb> &palette)
{
    if (colorNumber == 4)
    {
        palette.push_back(black);
        palette.push_back(magenta);
        palette.push_back(yellow);
        palette.push_back(cyan);
    }
    else if (colorNumber == 8)
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

void encode::convert_dec_to_bin(int dec, std::string &bin, int start_position, int length)
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

encode::encode(int colorNumber, int symbol_number)
{
    if (colorNumber != 4 && colorNumber != 8)
        colorNumber = DEFAULT_COLOR_NUMBER;
    if (symbol_number < 1 || symbol_number > MAX_SYMBOL_NUMBER)
        symbol_number = DEFAULT_SYMBOL_NUMBER;

    colorNumber = colorNumber;
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

    encodeSeq.resize(currSeqCounter + 1 + isShift);

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

bool encode::isDefaultMode()
{
    if (colorNumber == 8 && (symbolEccLevels[0] == 0 || symbolEccLevels[0] == DEFAULT_ECC_LEVEL))
        return true;
    return false;
}

int symbol::getMetadataLength(encode &enc)
{
    int length = 0;

    if (index == 0)
    {
        if (enc.isDefaultMode())
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

int symbol::getSymbolCapacity(encode &enc)
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
        int nb_metadata_bits = getMetadataLength(enc);
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

void symbol::getOptimalECC(int capacity, int net_data_length)
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

std::string encode::encodeData(std::string &data, int encodedLength)
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

bool encode::encodeMasterMetadata()
{
    int partI_length = MASTER_METADATA_PART1_LENGTH / 2;  // partI net length
    int partII_length = MASTER_METADATA_PART2_LENGTH / 2; // partII net length
    int V_length = 10;
    int E_length = 6;
    int MSK_length = 3;
    // set master metadata variables
    int Nc = log(colorNumber) / log(2.0) - 1;
    int V = ((symbolVersions[0].x - 1) << 5) + (symbolVersions[0].y - 1);
    int E1 = symbols[0].wcwr[0] - 3;
    int E2 = symbols[0].wcwr[1] - 4;
    int MSK = DEFAULT_MASKING_REFERENCE;

    // write each part of master metadata
    // Part I
    std::string partI;
    partI.resize(partI_length);
    convert_dec_to_bin(Nc, partI, 0, partI_length);
    // Part II
    std::string partII;
    partI.resize(partII_length);
    convert_dec_to_bin(V, partII, 0, V_length);
    convert_dec_to_bin(E1, partII, V_length, 3);
    convert_dec_to_bin(E2, partII, V_length + 3, 3);
    convert_dec_to_bin(MSK, partII, V_length + E_length, MSK_length);

    // encode each part of master metadata
    int wcwr[2] = {2, -1};
    // Part I
    std::string encoded_partI; //= encodeLDPC(partI, wcwr);

    // Part II
    std::string encoded_partII; //= encodeLDPC(partII, wcwr);

    int encoded_metadata_length = encoded_partI.length() + encoded_partII.length();

    // copy encoded parts into metadata
    symbols[0].metadata = partI;
    symbols[0].metadata += partII;
    return true;
}

bool encode::updateMasterMetadataPartII(int mask_ref)
{
    int partII_length = MASTER_METADATA_PART2_LENGTH / 2; // partII net length
    std::string partII;
    partII.resize(partII_length);

    // set V and E
    int V_length = 10;
    int E_length = 6;
    int MSK_length = 3;
    int V = ((symbolVersions[0].x - 1) << 5) + (symbolVersions[0].y - 1);
    int E1 = symbols[0].wcwr[0] - 3;
    int E2 = symbols[0].wcwr[1] - 4;
    convert_dec_to_bin(V, partII, 0, V_length);
    convert_dec_to_bin(E1, partII, V_length, 3);
    convert_dec_to_bin(E2, partII, V_length + 3, 3);

    // update masking reference in PartII
    convert_dec_to_bin(mask_ref, partII, V_length + E_length, MSK_length);

    // encode new PartII
    int wcwr[2] = {2, -1};
    std::string encoded_partII = encodeLDPC(partII, wcwr);
    // update metadata
    symbols[0].metadata.replace(MASTER_METADATA_PART1_LENGTH, encoded_partII.length(), encoded_partII);

    return true;
}

void encode::getNextMetadataModuleInMaster(int matrix_height, int matrix_width, int next_module_count, int *x, int *y)
{
    if (next_module_count % 4 == 0 || next_module_count % 4 == 2)
    {
        (*y) = matrix_height - 1 - (*y);
    }
    if (next_module_count % 4 == 1 || next_module_count % 4 == 3)
    {
        (*x) = matrix_width - 1 - (*x);
    }
    if (next_module_count % 4 == 0)
    {
        if (next_module_count <= 20 ||
            (next_module_count >= 44 && next_module_count <= 68) ||
            (next_module_count >= 96 && next_module_count <= 124) ||
            (next_module_count >= 156 && next_module_count <= 172))
        {
            (*y) += 1;
        }
        else if ((next_module_count > 20 && next_module_count < 44) ||
                 (next_module_count > 68 && next_module_count < 96) ||
                 (next_module_count > 124 && next_module_count < 156))
        {
            (*x) -= 1;
        }
    }
    if (next_module_count == 44 || next_module_count == 96 || next_module_count == 156)
    {
        int tmp = (*x);
        (*x) = (*y);
        (*y) = tmp;
    }
}

void encode::placeMasterMetadataPartII()
{
    // rewrite metadata in master with mask information
    int nb_of_bits_per_mod = log(colorNumber) / log(2);
    int x = MASTER_METADATA_X;
    int y = MASTER_METADATA_Y;
    int module_count = 0;
    // skip PartI and color palette
    int color_palette_size = MIN(colorNumber - 2, 64 - 2);
    int module_offset = MASTER_METADATA_PART1_MODULE_NUMBER + color_palette_size * COLOR_PALETTE_NUMBER;
    for (int i = 0; i < module_offset; i++)
    {
        module_count++;
        getNextMetadataModuleInMaster(symbols[0].side_size.y, symbols[0].side_size.x, module_count, &x, &y);
    }
    // update PartII
    int partII_bit_start = MASTER_METADATA_PART1_LENGTH;
    int partII_bit_end = MASTER_METADATA_PART1_LENGTH + MASTER_METADATA_PART2_LENGTH;
    int metadata_index = partII_bit_start;
    while (metadata_index <= partII_bit_end)
    {
        uint8_t color_index = symbols[0].matrix[y * symbols[0].side_size.x + x];
        for (int j = 0; j < nb_of_bits_per_mod; j++)
        {
            if (metadata_index <= partII_bit_end)
            {
                uint8_t bit = symbols[0].metadata[metadata_index];
                if (bit == 0)
                    color_index &= ~(1UL << (nb_of_bits_per_mod - 1 - j));
                else
                    color_index |= 1UL << (nb_of_bits_per_mod - 1 - j);
                metadata_index++;
            }
            else
                break;
        }
        symbols[0].matrix[y * symbols[0].side_size.x + x] = color_index;
        module_count++;
        getNextMetadataModuleInMaster(symbols[0].side_size.y, symbols[0].side_size.x, module_count, &x, &y);
    }
}

void getColorPaletteIndex(uint8_t *index, int index_size, int colorNumber)
{
    for (int i = 0; i < index_size; i++)
    {
        index[i] = i;
    }

    if (colorNumber < 128)
        return;

    uint8_t tmp[colorNumber];
    for (int i = 0; i < colorNumber; i++)
    {
        tmp[i] = i;
    }

    if (colorNumber == 128)
    {
        memcpy(index + 0, tmp + 0, 16);
        memcpy(index + 16, tmp + 32, 16);
        memcpy(index + 32, tmp + 80, 16);
        memcpy(index + 48, tmp + 112, 16);
    }
    else if (colorNumber == 256)
    {
        memcpy(index + 0, tmp + 0, 4);
        memcpy(index + 4, tmp + 8, 4);
        memcpy(index + 8, tmp + 20, 4);
        memcpy(index + 12, tmp + 28, 4);

        memcpy(index + 16, tmp + 64, 4);
        memcpy(index + 20, tmp + 72, 4);
        memcpy(index + 24, tmp + 84, 4);
        memcpy(index + 28, tmp + 92, 4);

        memcpy(index + 32, tmp + 160, 4);
        memcpy(index + 36, tmp + 168, 4);
        memcpy(index + 40, tmp + 180, 4);
        memcpy(index + 44, tmp + 188, 4);

        memcpy(index + 48, tmp + 224, 4);
        memcpy(index + 52, tmp + 232, 4);
        memcpy(index + 56, tmp + 244, 4);
        memcpy(index + 60, tmp + 252, 4);
    }
}

bool symbol::createMatrix(encode &enc, std::string &ecc_encoded_data)
{
    // Allocate matrix
    enc.symbols[index].matrix.assign(enc.symbols[index].side_size.x * enc.symbols[index].side_size.y, 0);
    // Allocate boolean matrix
    enc.symbols[index].dataMap.assign(enc.symbols[index].side_size.x * enc.symbols[index].side_size.y, 1);
    // set alignment patterns
    int Nc = log(enc.colorNumber) / log(2.0) - 1;
    uint8_t apx_core_color = apx_core_color_index[Nc];
    uint8_t apx_peri_color = apn_core_color_index[Nc];
    int side_ver_x_index = SIZE2VERSION(enc.symbols[index].side_size.x) - 1;
    int side_ver_y_index = SIZE2VERSION(enc.symbols[index].side_size.y) - 1;
    for (int x = 0; x < apNum[side_ver_x_index]; x++)
    {
        uint8_t left;
        if (x % 2 == 1)
            left = 0;
        else
            left = 1;
        for (int y = 0; y < apNum[side_ver_y_index]; y++)
        {
            int x_offset = apPos[side_ver_x_index][x] - 1;
            int y_offset = apPos[side_ver_y_index][y] - 1;
            // left alignment patterns
            if (left == 1 && (x != 0 || y != 0) && (x != 0 || y != apNum[side_ver_y_index] - 1) && (x != apNum[side_ver_x_index] - 1 || y != 0) && (x != apNum[side_ver_x_index] - 1 || y != apNum[side_ver_y_index] - 1))
            {
                enc.symbols[index].matrix[(y_offset - 1) * enc.symbols[index].side_size.x + x_offset - 1] =
                    enc.symbols[index].matrix[(y_offset - 1) * enc.symbols[index].side_size.x + x_offset] =
                        enc.symbols[index].matrix[(y_offset)*enc.symbols[index].side_size.x + x_offset - 1] =
                            enc.symbols[index].matrix[(y_offset)*enc.symbols[index].side_size.x + x_offset + 1] =
                                enc.symbols[index].matrix[(y_offset + 1) * enc.symbols[index].side_size.x + x_offset] =
                                    enc.symbols[index].matrix[(y_offset + 1) * enc.symbols[index].side_size.x + x_offset + 1] = apx_peri_color;
                enc.symbols[index].matrix[(y_offset)*enc.symbols[index].side_size.x + x_offset] = apx_core_color;

                enc.symbols[index].dataMap[(y_offset - 1) * enc.symbols[index].side_size.x + x_offset - 1] =
                    enc.symbols[index].dataMap[(y_offset - 1) * enc.symbols[index].side_size.x + x_offset] =
                        enc.symbols[index].dataMap[(y_offset)*enc.symbols[index].side_size.x + x_offset - 1] =
                            enc.symbols[index].dataMap[(y_offset)*enc.symbols[index].side_size.x + x_offset + 1] =
                                enc.symbols[index].dataMap[(y_offset + 1) * enc.symbols[index].side_size.x + x_offset] =
                                    enc.symbols[index].dataMap[(y_offset + 1) * enc.symbols[index].side_size.x + x_offset + 1] =
                                        enc.symbols[index].dataMap[(y_offset)*enc.symbols[index].side_size.x + x_offset] = 0;
            }
            // right alignment patterns
            else if (left == 0 && (x != 0 || y != 0) && (x != 0 || y != apNum[side_ver_y_index] - 1) && (x != apNum[side_ver_x_index] - 1 || y != 0) && (x != apNum[side_ver_x_index] - 1 || y != apNum[side_ver_y_index] - 1))
            {
                enc.symbols[index].matrix[(y_offset - 1) * enc.symbols[index].side_size.x + x_offset + 1] =
                    enc.symbols[index].matrix[(y_offset - 1) * enc.symbols[index].side_size.x + x_offset] =
                        enc.symbols[index].matrix[(y_offset)*enc.symbols[index].side_size.x + x_offset - 1] =
                            enc.symbols[index].matrix[(y_offset)*enc.symbols[index].side_size.x + x_offset + 1] =
                                enc.symbols[index].matrix[(y_offset + 1) * enc.symbols[index].side_size.x + x_offset] =
                                    enc.symbols[index].matrix[(y_offset + 1) * enc.symbols[index].side_size.x + x_offset - 1] = apx_peri_color;
                enc.symbols[index].matrix[(y_offset)*enc.symbols[index].side_size.x + x_offset] = apx_core_color;

                enc.symbols[index].dataMap[(y_offset - 1) * enc.symbols[index].side_size.x + x_offset + 1] =
                    enc.symbols[index].dataMap[(y_offset - 1) * enc.symbols[index].side_size.x + x_offset] =
                        enc.symbols[index].dataMap[(y_offset)*enc.symbols[index].side_size.x + x_offset - 1] =
                            enc.symbols[index].dataMap[(y_offset)*enc.symbols[index].side_size.x + x_offset + 1] =
                                enc.symbols[index].dataMap[(y_offset + 1) * enc.symbols[index].side_size.x + x_offset] =
                                    enc.symbols[index].dataMap[(y_offset + 1) * enc.symbols[index].side_size.x + x_offset - 1] =
                                        enc.symbols[index].dataMap[(y_offset)*enc.symbols[index].side_size.x + x_offset] = 0;
            }
            if (left == 0)
                left = 1;
            else
                left = 0;
        }
    }

    // outer layers of finder pattern for master symbol
    if (index == 0)
    {
        // if k=0 center, k=1 first layer, k=2 second layer
        for (int k = 0; k < 3; k++)
        {
            for (int i = 0; i < k + 1; i++)
            {
                for (int j = 0; j < k + 1; j++)
                {
                    if (i == k || j == k)
                    {
                        uint8_t fp0_color_index, fp1_color_index, fp2_color_index, fp3_color_index;
                        fp0_color_index = (k % 2) ? fp3_core_color_index[Nc] : fp0_core_color_index[Nc];
                        fp1_color_index = (k % 2) ? fp2_core_color_index[Nc] : fp1_core_color_index[Nc];
                        fp2_color_index = (k % 2) ? fp1_core_color_index[Nc] : fp2_core_color_index[Nc];
                        fp3_color_index = (k % 2) ? fp0_core_color_index[Nc] : fp3_core_color_index[Nc];

                        // upper pattern
                        enc.symbols[index].matrix[(DISTANCE_TO_BORDER - (i + 1)) * enc.symbols[index].side_size.x + DISTANCE_TO_BORDER - j - 1] =
                            enc.symbols[index].matrix[(DISTANCE_TO_BORDER + (i - 1)) * enc.symbols[index].side_size.x + DISTANCE_TO_BORDER + j - 1] = fp0_color_index;
                        enc.symbols[index].dataMap[(DISTANCE_TO_BORDER - (i + 1)) * enc.symbols[index].side_size.x + DISTANCE_TO_BORDER - j - 1] =
                            enc.symbols[index].dataMap[(DISTANCE_TO_BORDER + (i - 1)) * enc.symbols[index].side_size.x + DISTANCE_TO_BORDER + j - 1] = 0;

                        enc.symbols[index].matrix[(DISTANCE_TO_BORDER - (i + 1)) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            enc.symbols[index].matrix[(DISTANCE_TO_BORDER + (i - 1)) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = fp1_color_index;
                        enc.symbols[index].dataMap[(DISTANCE_TO_BORDER - (i + 1)) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            enc.symbols[index].dataMap[(DISTANCE_TO_BORDER + (i - 1)) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = 0;

                        // lower pattern
                        enc.symbols[index].matrix[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            enc.symbols[index].matrix[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = fp2_color_index;
                        enc.symbols[index].dataMap[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            enc.symbols[index].dataMap[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = 0;

                        enc.symbols[index].matrix[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * enc.symbols[index].side_size.x + (DISTANCE_TO_BORDER)-j - 1] =
                            enc.symbols[index].matrix[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * enc.symbols[index].side_size.x + (DISTANCE_TO_BORDER) + j - 1] = fp3_color_index;
                        enc.symbols[index].dataMap[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * enc.symbols[index].side_size.x + (DISTANCE_TO_BORDER)-j - 1] =
                            enc.symbols[index].dataMap[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * enc.symbols[index].side_size.x + (DISTANCE_TO_BORDER) + j - 1] = 0;
                    }
                }
            }
        }
    }
    else // finder alignments in slave
    {
        // if k=0 center, k=1 first layer
        for (int k = 0; k < 2; k++)
        {
            for (int i = 0; i < k + 1; i++)
            {
                for (int j = 0; j < k + 1; j++)
                {
                    if (i == k || j == k)
                    {
                        uint8_t ap0_color_index, ap1_color_index, ap2_color_index, ap3_color_index;
                        ap0_color_index =
                            ap1_color_index =
                                ap2_color_index =
                                    ap3_color_index = (k % 2) ? apx_core_color_index[Nc] : apn_core_color_index[Nc];
                        // upper pattern
                        enc.symbols[index].matrix[(DISTANCE_TO_BORDER - (i + 1)) * enc.symbols[index].side_size.x + DISTANCE_TO_BORDER - j - 1] =
                            enc.symbols[index].matrix[(DISTANCE_TO_BORDER + (i - 1)) * enc.symbols[index].side_size.x + DISTANCE_TO_BORDER + j - 1] = ap0_color_index;
                        enc.symbols[index].dataMap[(DISTANCE_TO_BORDER - (i + 1)) * enc.symbols[index].side_size.x + DISTANCE_TO_BORDER - j - 1] =
                            enc.symbols[index].dataMap[(DISTANCE_TO_BORDER + (i - 1)) * enc.symbols[index].side_size.x + DISTANCE_TO_BORDER + j - 1] = 0;

                        enc.symbols[index].matrix[(DISTANCE_TO_BORDER - (i + 1)) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            enc.symbols[index].matrix[(DISTANCE_TO_BORDER + (i - 1)) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = ap1_color_index;
                        enc.symbols[index].dataMap[(DISTANCE_TO_BORDER - (i + 1)) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            enc.symbols[index].dataMap[(DISTANCE_TO_BORDER + (i - 1)) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = 0;

                        // lower pattern
                        enc.symbols[index].matrix[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            enc.symbols[index].matrix[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = ap2_color_index;
                        enc.symbols[index].dataMap[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            enc.symbols[index].dataMap[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * enc.symbols[index].side_size.x + enc.symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = 0;

                        enc.symbols[index].matrix[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * enc.symbols[index].side_size.x + (DISTANCE_TO_BORDER)-j - 1] =
                            enc.symbols[index].matrix[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * enc.symbols[index].side_size.x + (DISTANCE_TO_BORDER) + j - 1] = ap3_color_index;
                        enc.symbols[index].dataMap[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * enc.symbols[index].side_size.x + (DISTANCE_TO_BORDER)-j - 1] =
                            enc.symbols[index].dataMap[(enc.symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * enc.symbols[index].side_size.x + (DISTANCE_TO_BORDER) + j - 1] = 0;
                    }
                }
            }
        }
    }

    // Metadata and color palette placement
    int nb_of_bits_per_mod = log(enc.colorNumber) / log(2);
    int color_index;
    int module_count = 0;
    int x;
    int y;

    // get color index for color palette
    int palette_index_size = enc.colorNumber > 64 ? 64 : enc.colorNumber;
    uint8_t palette_index[palette_index_size];
    getColorPaletteIndex(palette_index, palette_index_size, enc.colorNumber);

    if (index == 0) // place metadata and color palette in master symbol
    {
        x = MASTER_METADATA_X;
        y = MASTER_METADATA_Y;
        int metadata_index = 0;
        // metadata Part I
        if (!enc.isDefaultMode())
        {
            while (metadata_index < enc.symbols[index].metadata.length() && metadata_index < MASTER_METADATA_PART1_LENGTH)
            {
                // Read 3 bits from encoded PartI each time
                uint8_t bit1 = enc.symbols[index].metadata[metadata_index + 0];
                uint8_t bit2 = enc.symbols[index].metadata[metadata_index + 1];
                uint8_t bit3 = enc.symbols[index].metadata[metadata_index + 2];
                int val = (bit1 << 2) + (bit2 << 1) + bit3;
                // place two modules according to the value of every 3 bits
                for (int i = 0; i < 2; i++)
                {
                    color_index = nc_color_encode_table[val][i] % enc.colorNumber;
                    enc.symbols[index].matrix[y * enc.symbols[index].side_size.x + x] = color_index;
                    enc.symbols[index].dataMap[y * enc.symbols[index].side_size.x + x] = 0;
                    module_count++;
                    enc.getNextMetadataModuleInMaster(enc.symbols[index].side_size.y, enc.symbols[index].side_size.x, module_count, &x, &y);
                }
                metadata_index += 3;
            }
        }
        // color palette
        for (int i = 2; i < MIN(enc.colorNumber, 64); i++) // skip the first two colors in finder pattern
        {
            enc.symbols[index].matrix[y * enc.symbols[index].side_size.x + x] = palette_index[master_palette_placement_index[0][i] % enc.colorNumber];
            enc.symbols[index].dataMap[y * enc.symbols[index].side_size.x + x] = 0;
            module_count++;
            enc.getNextMetadataModuleInMaster(enc.symbols[index].side_size.y, enc.symbols[index].side_size.x, module_count, &x, &y);

            enc.symbols[index].matrix[y * enc.symbols[index].side_size.x + x] = palette_index[master_palette_placement_index[1][i] % enc.colorNumber];
            enc.symbols[index].dataMap[y * enc.symbols[index].side_size.x + x] = 0;
            module_count++;
            enc.getNextMetadataModuleInMaster(enc.symbols[index].side_size.y, enc.symbols[index].side_size.x, module_count, &x, &y);

            enc.symbols[index].matrix[y * enc.symbols[index].side_size.x + x] = palette_index[master_palette_placement_index[2][i] % enc.colorNumber];
            enc.symbols[index].dataMap[y * enc.symbols[index].side_size.x + x] = 0;
            module_count++;
            enc.getNextMetadataModuleInMaster(enc.symbols[index].side_size.y, enc.symbols[index].side_size.x, module_count, &x, &y);

            enc.symbols[index].matrix[y * enc.symbols[index].side_size.x + x] = palette_index[master_palette_placement_index[3][i] % enc.colorNumber];
            enc.symbols[index].dataMap[y * enc.symbols[index].side_size.x + x] = 0;
            module_count++;
            enc.getNextMetadataModuleInMaster(enc.symbols[index].side_size.y, enc.symbols[index].side_size.x, module_count, &x, &y);
        }
        // metadata PartII
        if (!enc.isDefaultMode())
        {
            while (metadata_index < enc.symbols[index].metadata.length())
            {
                color_index = 0;
                for (int j = 0; j < nb_of_bits_per_mod; j++)
                {
                    if (metadata_index < enc.symbols[index].metadata.length())
                    {
                        color_index += ((int)enc.symbols[index].metadata[metadata_index]) << (nb_of_bits_per_mod - 1 - j);
                        metadata_index++;
                    }
                    else
                        break;
                }
                enc.symbols[index].matrix[y * enc.symbols[index].side_size.x + x] = color_index;
                enc.symbols[index].dataMap[y * enc.symbols[index].side_size.x + x] = 0;
                module_count++;
                enc.getNextMetadataModuleInMaster(enc.symbols[index].side_size.y, enc.symbols[index].side_size.x, module_count, &x, &y);
            }
        }
    }
    else // place color palette in slave symbol
    {
        // color palette
        int width = enc.symbols[index].side_size.x;
        int height = enc.symbols[index].side_size.y;
        for (int i = 2; i < MIN(enc.colorNumber, 64); i++) // skip the first two colors in alignment pattern
        {
            // left
            enc.symbols[index].matrix[slave_palette_position[i - 2].y * width + slave_palette_position[i - 2].x] = palette_index[slave_palette_placement_index[i] % enc.colorNumber];
            enc.symbols[index].dataMap[slave_palette_position[i - 2].y * width + slave_palette_position[i - 2].x] = 0;
            // top
            enc.symbols[index].matrix[slave_palette_position[i - 2].x * width + (width - 1 - slave_palette_position[i - 2].y)] = palette_index[slave_palette_placement_index[i] % enc.colorNumber];
            enc.symbols[index].dataMap[slave_palette_position[i - 2].x * width + (width - 1 - slave_palette_position[i - 2].y)] = 0;
            // right
            enc.symbols[index].matrix[(height - 1 - slave_palette_position[i - 2].y) * width + (width - 1 - slave_palette_position[i - 2].x)] = palette_index[slave_palette_placement_index[i] % enc.colorNumber];
            enc.symbols[index].dataMap[(height - 1 - slave_palette_position[i - 2].y) * width + (width - 1 - slave_palette_position[i - 2].x)] = 0;
            // bottom
            enc.symbols[index].matrix[(height - 1 - slave_palette_position[i - 2].x) * width + slave_palette_position[i - 2].y] = palette_index[slave_palette_placement_index[i] % enc.colorNumber];
            enc.symbols[index].dataMap[(height - 1 - slave_palette_position[i - 2].x) * width + slave_palette_position[i - 2].y] = 0;
        }
    }

    int written_mess_part = 0;
    int padding = 0;
    for (int start_i = 0; start_i < enc.symbols[index].side_size.x; start_i++)
    {
        for (int i = start_i; i < enc.symbols[index].side_size.x * enc.symbols[index].side_size.y; i = i + enc.symbols[index].side_size.x)
        {
            if (enc.symbols[index].dataMap[i] != 0 && written_mess_part < ecc_encoded_data.length())
            {
                color_index = 0;
                for (int j = 0; j < nb_of_bits_per_mod; j++)
                {
                    if (written_mess_part < ecc_encoded_data.length())
                        color_index += ((int)ecc_encoded_data[written_mess_part]) << (nb_of_bits_per_mod - 1 - j); //*pow(2,nb_of_bits_per_mod-1-j);
                    else
                    {
                        color_index += padding << (nb_of_bits_per_mod - 1 - j); //*pow(2,nb_of_bits_per_mod-1-j);
                        if (padding == 0)
                            padding = 1;
                        else
                            padding = 0;
                    }
                    written_mess_part++;
                }
                enc.symbols[index].matrix[i] = (char)color_index;
            }
            else if (enc.symbols[index].dataMap[i] != 0) // write padding bits
            {
                color_index = 0;
                for (int j = 0; j < nb_of_bits_per_mod; j++)
                {
                    color_index += padding << (nb_of_bits_per_mod - 1 - j); //*pow(2,nb_of_bits_per_mod-1-j);
                    if (padding == 0)
                        padding = 1;
                    else
                        padding = 0;
                }
                enc.symbols[index].matrix[i] = (char)color_index; // i % enc.colorNumber;
            }
        }
    }
    return true;
}

int main()
{
    return 0;
}