
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

void encode::setDefaultEccLevels(int symbolNumber, std::vector<uint8_t> &ecc_levels)
{
    for (int i = 0; i < symbolNumber; i++)
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

encode::encode(int colorNumber, int symbolNumber)
{
    if (colorNumber != 4 && colorNumber != 8)
        colorNumber = DEFAULT_COLOR_NUMBER;
    if (symbolNumber < 1 || symbolNumber > MAX_SYMBOL_NUMBER)
        symbolNumber = DEFAULT_SYMBOL_NUMBER;

    colorNumber = colorNumber;
    symbolNumber = symbolNumber;

    setDefaultPalette(colorNumber, palette);

    setDefaultEccLevels(symbolNumber, symbolEccLevels);
}

std::vector<int> encode::analyzeInputData(std::string &input)
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
    encodedLength = encodeSeqLength;
    return encodeSeq;
}

bool encode::isDefaultMode()
{
    if (colorNumber == 8 && (symbolEccLevels[0] == 0 || symbolEccLevels[0] == DEFAULT_ECC_LEVEL))
        return true;
    return false;
}

int encode::getMetadataLength(int index)
{
    int length = 0;

    if (index == 0)
    {
        if (isDefaultMode())
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
        int host_index = symbols[index].host;
        if (symbolVersions[index].x != symbolVersions[host_index].x || symbolVersions[index].y != symbolVersions[host_index].y)
            length += 5;
        if (symbolEccLevels[index] != symbolEccLevels[host_index])
        {
            length += 6;
        }
    }
    return length;
}

int encode::getSymbolCapacity(int index)
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

    int nb_modules_palette = colorNumber > 64 ? (64 - 2) * COLOR_PALETTE_NUMBER : (colorNumber - 2) * COLOR_PALETTE_NUMBER;
    int side_size_x = VERSION2SIZE(symbolVersions[index].x);
    int side_size_y = VERSION2SIZE(symbolVersions[index].y);
    int number_of_aps_x = apNum[symbolVersions[index].x - 1];
    int number_of_aps_y = apNum[symbolVersions[index].y - 1];
    int nb_modules_ap = (number_of_aps_x * number_of_aps_y - 4) * 7;
    int nb_of_bpm = log(colorNumber) / log(2);
    int nb_modules_metadata = 0;
    if (index == 0)
    {
        int nb_metadata_bits = getMetadataLength(index);
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

void encode::encodeData(std::string &data)
{

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
    std::vector<int> swcwr{wcwr[0], wcwr[1]};
    std::string encoded_partI = encodeLDPC(partI, swcwr);

    // Part II
    std::string encoded_partII = encodeLDPC(partII, swcwr);

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
    std::vector<int> swcwr{wcwr[0], wcwr[1]};
    std::string encoded_partII = encodeLDPC(partII, swcwr);
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

bool encode::createMatrix(int index, std::string &ecc_encoded_data)
{
    // Allocate matrix
    symbols[index].matrix.assign(symbols[index].side_size.x * symbols[index].side_size.y, 0);
    // Allocate boolean matrix
    symbols[index].dataMap.assign(symbols[index].side_size.x * symbols[index].side_size.y, 1);
    // set alignment patterns
    int Nc = log(colorNumber) / log(2.0) - 1;
    uint8_t apx_core_color = apx_core_color_index[Nc];
    uint8_t apx_peri_color = apn_core_color_index[Nc];
    int side_ver_x_index = SIZE2VERSION(symbols[index].side_size.x) - 1;
    int side_ver_y_index = SIZE2VERSION(symbols[index].side_size.y) - 1;
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
                symbols[index].matrix[(y_offset - 1) * symbols[index].side_size.x + x_offset - 1] =
                    symbols[index].matrix[(y_offset - 1) * symbols[index].side_size.x + x_offset] =
                        symbols[index].matrix[(y_offset)*symbols[index].side_size.x + x_offset - 1] =
                            symbols[index].matrix[(y_offset)*symbols[index].side_size.x + x_offset + 1] =
                                symbols[index].matrix[(y_offset + 1) * symbols[index].side_size.x + x_offset] =
                                    symbols[index].matrix[(y_offset + 1) * symbols[index].side_size.x + x_offset + 1] = apx_peri_color;
                symbols[index].matrix[(y_offset)*symbols[index].side_size.x + x_offset] = apx_core_color;

                symbols[index].dataMap[(y_offset - 1) * symbols[index].side_size.x + x_offset - 1] =
                    symbols[index].dataMap[(y_offset - 1) * symbols[index].side_size.x + x_offset] =
                        symbols[index].dataMap[(y_offset)*symbols[index].side_size.x + x_offset - 1] =
                            symbols[index].dataMap[(y_offset)*symbols[index].side_size.x + x_offset + 1] =
                                symbols[index].dataMap[(y_offset + 1) * symbols[index].side_size.x + x_offset] =
                                    symbols[index].dataMap[(y_offset + 1) * symbols[index].side_size.x + x_offset + 1] =
                                        symbols[index].dataMap[(y_offset)*symbols[index].side_size.x + x_offset] = 0;
            }
            // right alignment patterns
            else if (left == 0 && (x != 0 || y != 0) && (x != 0 || y != apNum[side_ver_y_index] - 1) && (x != apNum[side_ver_x_index] - 1 || y != 0) && (x != apNum[side_ver_x_index] - 1 || y != apNum[side_ver_y_index] - 1))
            {
                symbols[index].matrix[(y_offset - 1) * symbols[index].side_size.x + x_offset + 1] =
                    symbols[index].matrix[(y_offset - 1) * symbols[index].side_size.x + x_offset] =
                        symbols[index].matrix[(y_offset)*symbols[index].side_size.x + x_offset - 1] =
                            symbols[index].matrix[(y_offset)*symbols[index].side_size.x + x_offset + 1] =
                                symbols[index].matrix[(y_offset + 1) * symbols[index].side_size.x + x_offset] =
                                    symbols[index].matrix[(y_offset + 1) * symbols[index].side_size.x + x_offset - 1] = apx_peri_color;
                symbols[index].matrix[(y_offset)*symbols[index].side_size.x + x_offset] = apx_core_color;

                symbols[index].dataMap[(y_offset - 1) * symbols[index].side_size.x + x_offset + 1] =
                    symbols[index].dataMap[(y_offset - 1) * symbols[index].side_size.x + x_offset] =
                        symbols[index].dataMap[(y_offset)*symbols[index].side_size.x + x_offset - 1] =
                            symbols[index].dataMap[(y_offset)*symbols[index].side_size.x + x_offset + 1] =
                                symbols[index].dataMap[(y_offset + 1) * symbols[index].side_size.x + x_offset] =
                                    symbols[index].dataMap[(y_offset + 1) * symbols[index].side_size.x + x_offset - 1] =
                                        symbols[index].dataMap[(y_offset)*symbols[index].side_size.x + x_offset] = 0;
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
                        symbols[index].matrix[(DISTANCE_TO_BORDER - (i + 1)) * symbols[index].side_size.x + DISTANCE_TO_BORDER - j - 1] =
                            symbols[index].matrix[(DISTANCE_TO_BORDER + (i - 1)) * symbols[index].side_size.x + DISTANCE_TO_BORDER + j - 1] = fp0_color_index;
                        symbols[index].dataMap[(DISTANCE_TO_BORDER - (i + 1)) * symbols[index].side_size.x + DISTANCE_TO_BORDER - j - 1] =
                            symbols[index].dataMap[(DISTANCE_TO_BORDER + (i - 1)) * symbols[index].side_size.x + DISTANCE_TO_BORDER + j - 1] = 0;

                        symbols[index].matrix[(DISTANCE_TO_BORDER - (i + 1)) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            symbols[index].matrix[(DISTANCE_TO_BORDER + (i - 1)) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = fp1_color_index;
                        symbols[index].dataMap[(DISTANCE_TO_BORDER - (i + 1)) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            symbols[index].dataMap[(DISTANCE_TO_BORDER + (i - 1)) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = 0;

                        // lower pattern
                        symbols[index].matrix[(symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            symbols[index].matrix[(symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = fp2_color_index;
                        symbols[index].dataMap[(symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            symbols[index].dataMap[(symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = 0;

                        symbols[index].matrix[(symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * symbols[index].side_size.x + (DISTANCE_TO_BORDER)-j - 1] =
                            symbols[index].matrix[(symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * symbols[index].side_size.x + (DISTANCE_TO_BORDER) + j - 1] = fp3_color_index;
                        symbols[index].dataMap[(symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * symbols[index].side_size.x + (DISTANCE_TO_BORDER)-j - 1] =
                            symbols[index].dataMap[(symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * symbols[index].side_size.x + (DISTANCE_TO_BORDER) + j - 1] = 0;
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
                        symbols[index].matrix[(DISTANCE_TO_BORDER - (i + 1)) * symbols[index].side_size.x + DISTANCE_TO_BORDER - j - 1] =
                            symbols[index].matrix[(DISTANCE_TO_BORDER + (i - 1)) * symbols[index].side_size.x + DISTANCE_TO_BORDER + j - 1] = ap0_color_index;
                        symbols[index].dataMap[(DISTANCE_TO_BORDER - (i + 1)) * symbols[index].side_size.x + DISTANCE_TO_BORDER - j - 1] =
                            symbols[index].dataMap[(DISTANCE_TO_BORDER + (i - 1)) * symbols[index].side_size.x + DISTANCE_TO_BORDER + j - 1] = 0;

                        symbols[index].matrix[(DISTANCE_TO_BORDER - (i + 1)) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            symbols[index].matrix[(DISTANCE_TO_BORDER + (i - 1)) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = ap1_color_index;
                        symbols[index].dataMap[(DISTANCE_TO_BORDER - (i + 1)) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            symbols[index].dataMap[(DISTANCE_TO_BORDER + (i - 1)) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = 0;

                        // lower pattern
                        symbols[index].matrix[(symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            symbols[index].matrix[(symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = ap2_color_index;
                        symbols[index].dataMap[(symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) - j - 1] =
                            symbols[index].dataMap[(symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * symbols[index].side_size.x + symbols[index].side_size.x - (DISTANCE_TO_BORDER - 1) + j - 1] = 0;

                        symbols[index].matrix[(symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * symbols[index].side_size.x + (DISTANCE_TO_BORDER)-j - 1] =
                            symbols[index].matrix[(symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * symbols[index].side_size.x + (DISTANCE_TO_BORDER) + j - 1] = ap3_color_index;
                        symbols[index].dataMap[(symbols[index].side_size.y - DISTANCE_TO_BORDER + i) * symbols[index].side_size.x + (DISTANCE_TO_BORDER)-j - 1] =
                            symbols[index].dataMap[(symbols[index].side_size.y - DISTANCE_TO_BORDER - i) * symbols[index].side_size.x + (DISTANCE_TO_BORDER) + j - 1] = 0;
                    }
                }
            }
        }
    }

    // Metadata and color palette placement
    int nb_of_bits_per_mod = log(colorNumber) / log(2);
    int color_index;
    int module_count = 0;
    int x;
    int y;

    // get color index for color palette
    int palette_index_size = colorNumber > 64 ? 64 : colorNumber;
    uint8_t palette_index[palette_index_size];
    getColorPaletteIndex(palette_index, palette_index_size, colorNumber);

    if (index == 0) // place metadata and color palette in master symbol
    {
        x = MASTER_METADATA_X;
        y = MASTER_METADATA_Y;
        int metadata_index = 0;
        // metadata Part I
        if (!isDefaultMode())
        {
            while (metadata_index < symbols[index].metadata.length() && metadata_index < MASTER_METADATA_PART1_LENGTH)
            {
                // Read 3 bits from encoded PartI each time
                uint8_t bit1 = symbols[index].metadata[metadata_index + 0];
                uint8_t bit2 = symbols[index].metadata[metadata_index + 1];
                uint8_t bit3 = symbols[index].metadata[metadata_index + 2];
                int val = (bit1 << 2) + (bit2 << 1) + bit3;
                // place two modules according to the value of every 3 bits
                for (int i = 0; i < 2; i++)
                {
                    color_index = nc_color_encode_table[val][i] % colorNumber;
                    symbols[index].matrix[y * symbols[index].side_size.x + x] = color_index;
                    symbols[index].dataMap[y * symbols[index].side_size.x + x] = 0;
                    module_count++;
                    getNextMetadataModuleInMaster(symbols[index].side_size.y, symbols[index].side_size.x, module_count, &x, &y);
                }
                metadata_index += 3;
            }
        }
        // color palette
        for (int i = 2; i < MIN(colorNumber, 64); i++) // skip the first two colors in finder pattern
        {
            symbols[index].matrix[y * symbols[index].side_size.x + x] = palette_index[master_palette_placement_index[0][i] % colorNumber];
            symbols[index].dataMap[y * symbols[index].side_size.x + x] = 0;
            module_count++;
            getNextMetadataModuleInMaster(symbols[index].side_size.y, symbols[index].side_size.x, module_count, &x, &y);

            symbols[index].matrix[y * symbols[index].side_size.x + x] = palette_index[master_palette_placement_index[1][i] % colorNumber];
            symbols[index].dataMap[y * symbols[index].side_size.x + x] = 0;
            module_count++;
            getNextMetadataModuleInMaster(symbols[index].side_size.y, symbols[index].side_size.x, module_count, &x, &y);

            symbols[index].matrix[y * symbols[index].side_size.x + x] = palette_index[master_palette_placement_index[2][i] % colorNumber];
            symbols[index].dataMap[y * symbols[index].side_size.x + x] = 0;
            module_count++;
            getNextMetadataModuleInMaster(symbols[index].side_size.y, symbols[index].side_size.x, module_count, &x, &y);

            symbols[index].matrix[y * symbols[index].side_size.x + x] = palette_index[master_palette_placement_index[3][i] % colorNumber];
            symbols[index].dataMap[y * symbols[index].side_size.x + x] = 0;
            module_count++;
            getNextMetadataModuleInMaster(symbols[index].side_size.y, symbols[index].side_size.x, module_count, &x, &y);
        }
        // metadata PartII
        if (!isDefaultMode())
        {
            while (metadata_index < symbols[index].metadata.length())
            {
                color_index = 0;
                for (int j = 0; j < nb_of_bits_per_mod; j++)
                {
                    if (metadata_index < symbols[index].metadata.length())
                    {
                        color_index += ((int)symbols[index].metadata[metadata_index]) << (nb_of_bits_per_mod - 1 - j);
                        metadata_index++;
                    }
                    else
                        break;
                }
                symbols[index].matrix[y * symbols[index].side_size.x + x] = color_index;
                symbols[index].dataMap[y * symbols[index].side_size.x + x] = 0;
                module_count++;
                getNextMetadataModuleInMaster(symbols[index].side_size.y, symbols[index].side_size.x, module_count, &x, &y);
            }
        }
    }
    else // place color palette in slave symbol
    {
        // color palette
        int width = symbols[index].side_size.x;
        int height = symbols[index].side_size.y;
        for (int i = 2; i < MIN(colorNumber, 64); i++) // skip the first two colors in alignment pattern
        {
            // left
            symbols[index].matrix[slave_palette_position[i - 2].y * width + slave_palette_position[i - 2].x] = palette_index[slave_palette_placement_index[i] % colorNumber];
            symbols[index].dataMap[slave_palette_position[i - 2].y * width + slave_palette_position[i - 2].x] = 0;
            // top
            symbols[index].matrix[slave_palette_position[i - 2].x * width + (width - 1 - slave_palette_position[i - 2].y)] = palette_index[slave_palette_placement_index[i] % colorNumber];
            symbols[index].dataMap[slave_palette_position[i - 2].x * width + (width - 1 - slave_palette_position[i - 2].y)] = 0;
            // right
            symbols[index].matrix[(height - 1 - slave_palette_position[i - 2].y) * width + (width - 1 - slave_palette_position[i - 2].x)] = palette_index[slave_palette_placement_index[i] % colorNumber];
            symbols[index].dataMap[(height - 1 - slave_palette_position[i - 2].y) * width + (width - 1 - slave_palette_position[i - 2].x)] = 0;
            // bottom
            symbols[index].matrix[(height - 1 - slave_palette_position[i - 2].x) * width + slave_palette_position[i - 2].y] = palette_index[slave_palette_placement_index[i] % colorNumber];
            symbols[index].dataMap[(height - 1 - slave_palette_position[i - 2].x) * width + slave_palette_position[i - 2].y] = 0;
        }
    }

    int written_mess_part = 0;
    int padding = 0;
    for (int start_i = 0; start_i < symbols[index].side_size.x; start_i++)
    {
        for (int i = start_i; i < symbols[index].side_size.x * symbols[index].side_size.y; i = i + symbols[index].side_size.x)
        {
            if (symbols[index].dataMap[i] != 0 && written_mess_part < ecc_encoded_data.length())
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
                symbols[index].matrix[i] = (char)color_index;
            }
            else if (symbols[index].dataMap[i] != 0) // write padding bits
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
                symbols[index].matrix[i] = (char)color_index; // i % colorNumber;
            }
        }
    }
    return true;
}

void encode::swap_symbols(int index1, int index2)
{
    swap_int(&symbolPositions[index1], &symbolPositions[index2]);
    swap_int(&symbolVersions[index1].x, &symbolVersions[index2].x);
    swap_int(&symbolVersions[index1].y, &symbolVersions[index2].y);
    swap_byte(&symbolEccLevels[index1], &symbolEccLevels[index2]);
    // swap symbols
    symbol s;
    s = symbols[index1];
    symbols[index1] = symbols[index2];
    symbols[index2] = s;
}

bool encode::assignDockedSymbols()
{
    // initialize host and slaves
    for (int i = 0; i < symbolNumber; i++)
    {
        // initialize symbol host index
        symbols[i].host = -1;
        // initialize symbol's slave index
        for (int j = 0; j < 4; j++)
            symbols[i].slaves[j] = 0; // 0:no slave
    }
    // assign docked symbols
    int assigned_slave_index = 1;
    for (int i = 0; i < symbolNumber - 1 && assigned_slave_index < symbolNumber; i++)
    {
        for (int j = 0; j < 4 && assigned_slave_index < symbolNumber; j++)
        {
            for (int k = i + 1; k < symbolNumber && assigned_slave_index < symbolNumber; k++)
            {
                if (symbols[k].host == -1)
                {
                    int hpos = symbolPositions[i];
                    int spos = symbolPositions[k];
                    bool slave_found = false;
                    switch (j)
                    {
                    case 0: // top
                        if (symbolPos[hpos].x == symbolPos[spos].x && symbolPos[hpos].y - 1 == symbolPos[spos].y)
                        {
                            symbols[i].slaves[0] = assigned_slave_index;
                            symbols[k].slaves[1] = -1; //-1:host position
                            slave_found = true;
                        }
                        break;
                    case 1: // bottom
                        if (symbolPos[hpos].x == symbolPos[spos].x && symbolPos[hpos].y + 1 == symbolPos[spos].y)
                        {
                            symbols[i].slaves[1] = assigned_slave_index;
                            symbols[k].slaves[0] = -1;
                            slave_found = true;
                        }
                        break;
                    case 2: // left
                        if (symbolPos[hpos].y == symbolPos[spos].y && symbolPos[hpos].x - 1 == symbolPos[spos].x)
                        {
                            symbols[i].slaves[2] = assigned_slave_index;
                            symbols[k].slaves[3] = -1;
                            slave_found = true;
                        }
                        break;
                    case 3: // right
                        if (symbolPos[hpos].y == symbolPos[spos].y && symbolPos[hpos].x + 1 == symbolPos[spos].x)
                        {
                            symbols[i].slaves[3] = assigned_slave_index;
                            symbols[k].slaves[2] = -1;
                            slave_found = true;
                        }
                        break;
                    }
                    if (slave_found)
                    {
                        swap_symbols(k, assigned_slave_index);
                        symbols[assigned_slave_index].host = i;
                        assigned_slave_index++;
                    }
                }
            }
        }
    }

    // check if there is undocked symbol
    for (int i = 1; i < symbolNumber; i++)
    {
        if (symbols[i].host == -1)
        {
            std::cout << "Slave symbol at position " << symbolPositions[i] << " has no host\n";
            return false;
        }
    }
    return true;
}

void encode::getCodePara()
{
    // calculate the module size in pixel
    if (masterSymbolWidth != 0 || masterSymbolHeight != 0)
    {
        int dimension_x = masterSymbolWidth / symbols[0].side_size.x;
        int dimension_y = masterSymbolHeight / symbols[0].side_size.y;
        p.dimension = dimension_x > dimension_y ? dimension_x : dimension_y;
        if (p.dimension < 1)
            p.dimension = 1;
    }
    else
    {
        p.dimension = moduleSize;
    }

    // find the coordinate range of symbols
    p.min_x = 0;
    p.min_y = 0;
    int max_x = 0, max_y = 0;
    for (int i = 0; i < symbolNumber; i++)
    {
        // find the mininal x and y
        if (symbolPos[symbolPositions[i]].x < p.min_x)
            p.min_x = symbolPos[symbolPositions[i]].x;
        if (symbolPos[symbolPositions[i]].y < p.min_y)
            p.min_y = symbolPos[symbolPositions[i]].y;
        // find the maximal x and y
        if (symbolPos[symbolPositions[i]].x > max_x)
            max_x = symbolPos[symbolPositions[i]].x;
        if (symbolPos[symbolPositions[i]].y > max_y)
            max_y = symbolPos[symbolPositions[i]].y;
    }

    // calculate the code size
    p.rows = max_y - p.min_y + 1;
    p.cols = max_x - p.min_x + 1;
    p.row_height.resize(p.rows);
    p.col_width.resize(p.cols);

    p.code_size.x = 0;
    p.code_size.y = 0;
    bool flag = 0;
    for (int x = p.min_x; x <= max_x; x++)
    {
        flag = 0;
        for (int i = 0; i < symbolNumber; i++)
        {
            if (symbolPos[symbolPositions[i]].x == x)
            {
                p.col_width[x - p.min_x] = symbols[i].side_size.x;
                p.code_size.x += p.col_width[x - p.min_x];
                flag = 1;
            }
            if (flag)
                break;
        }
    }
    for (int y = p.min_y; y <= max_y; y++)
    {
        flag = 0;
        for (int i = 0; i < symbolNumber; i++)
        {
            if (symbolPos[symbolPositions[i]].y == y)
            {
                p.row_height[y - p.min_y] = symbols[i].side_size.y;
                p.code_size.y += p.row_height[y - p.min_y];
                flag = 1;
            }
            if (flag)
                break;
        }
    }
}

bool encode::createBitmap()
{
    // create bitmp
    int width = p.dimension * p.code_size.x;
    int height = p.dimension * p.code_size.y;
    int bytes_per_pixel = BITMAP_BITS_PER_PIXEL / 8;
    int bytes_per_row = width * bytes_per_pixel;

    bitmp.width = width;
    bitmp.height = height;
    bitmp.bits_per_pixel = BITMAP_BITS_PER_PIXEL;
    bitmp.bits_per_channel = BITMAP_BITS_PER_CHANNEL;
    bitmp.channel_count = BITMAP_CHANNEL_COUNT;

    // place symbols in bitmp
    for (int k = 0; k < symbolNumber; k++)
    {
        // calculate the starting coordinates of the symbol matrix
        int startx = 0, starty = 0;
        int col = symbolPos[symbolPositions[k]].x - p.min_x;
        int row = symbolPos[symbolPositions[k]].y - p.min_y;
        for (int c = 0; c < col; c++)
            startx += p.col_width[c];
        for (int r = 0; r < row; r++)
            starty += p.row_height[r];

        // place symbol in the code
        int symbol_width = symbols[k].side_size.x;
        int symbol_height = symbols[k].side_size.y;
        for (int x = startx; x < (startx + symbol_width); x++)
        {
            for (int y = starty; y < (starty + symbol_height); y++)
            {
                // place one module in the bitmp
                int p_index = symbols[k].matrix[(y - starty) * symbol_width + (x - startx)];
                for (int i = y * p.dimension; i < (y * p.dimension + p.dimension); i++)
                {
                    for (int j = x * p.dimension; j < (x * p.dimension + p.dimension); j++)
                    {
                        bitmp.pixel[i * bytes_per_row + j * bytes_per_pixel] = palette[p_index].R;     // R
                        bitmp.pixel[i * bytes_per_row + j * bytes_per_pixel + 1] = palette[p_index].B; // B
                        bitmp.pixel[i * bytes_per_row + j * bytes_per_pixel + 2] = palette[p_index].G; // G
                        bitmp.pixel[i * bytes_per_row + j * bytes_per_pixel + 3] = 255;                // A
                    }
                }
            }
        }
    }
    return true;
}

bool encode::checkDockedSymbolSize()
{
    for (int i = 0; i < symbolNumber; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int slave_index = symbols[i].slaves[j];
            if (slave_index > 0)
            {
                int hpos = symbolPositions[i];
                int spos = symbolPositions[slave_index];
                int x_diff = symbolPos[hpos].x - symbolPos[spos].x;
                int y_diff = symbolPos[hpos].y - symbolPos[spos].y;

                if (x_diff == 0 && symbolVersions[i].x != symbolVersions[slave_index].x)
                {
                    std::cout << "Slave symbol at position " << spos
                              << " has different side version in X direction as its host symbol at position " << hpos << "\n";

                    return false;
                }
                if (y_diff == 0 && symbolVersions[i].y != symbolVersions[slave_index].y)
                {
                    std::cout << "Slave symbol at position " << spos
                              << " has different side version in Y direction as its host symbol at position " << hpos << "\n";

                    return false;
                }
            }
        }
    }
    return true;
}

bool encode::setMasterSymbolVersion()
{
    // calculate required number of data modules depending on data_length
    int net_data_length = encodedData.length();
    int payload_length = net_data_length + 5; // plus S and flag bit
    if (symbolEccLevels[0] == 0)
        symbolEccLevels[0] = DEFAULT_ECC_LEVEL;
    symbols[0].wcwr[0] = ecclevel2wcwr[symbolEccLevels[0]][0];
    symbols[0].wcwr[1] = ecclevel2wcwr[symbolEccLevels[0]][1];

    // determine the minimum square symbol to fit data
    int capacity, net_capacity;
    bool found_flag = false;
    for (int i = 1; i <= 32; i++)
    {
        symbolVersions[0].x = i;
        symbolVersions[0].y = i;
        capacity = getSymbolCapacity(0);
        net_capacity = (capacity / symbols[0].wcwr[1]) * symbols[0].wcwr[1] - (capacity / symbols[0].wcwr[1]) * symbols[0].wcwr[0];
        if (net_capacity >= payload_length)
        {
            found_flag = true;
            break;
        }
    }
    if (!found_flag)
    {
        int level = -1;
        for (int j = (int)symbolEccLevels[0] - 1; j > 0; j--)
        {
            net_capacity = (capacity / ecclevel2wcwr[j][1]) * ecclevel2wcwr[j][1] - (capacity / ecclevel2wcwr[j][1]) * ecclevel2wcwr[j][0];
            if (net_capacity >= payload_length)
                level = j;
        }
        if (level > 0)
        {
            std::cout << "Message does not fit into one symbol with the given ECC level. Please use an ECC level lower than " << level << " with '--ecc-level " << level << "\n";
            return false;
        }
        else
        {
            std::cout << "Message does not fit into one symbol. Use more symbols.\n";
            return false;
        }
    }
    // update symbol side size
    symbols[0].side_size.x = VERSION2SIZE(symbolVersions[0].x);
    symbols[0].side_size.y = VERSION2SIZE(symbolVersions[0].y);

    return true;
}

bool encode::addE2SlaveMetadata(symbol &slave)
{
    // copy old metadata to new metadata
    int old_metadata_length = slave.metadata.length();
    int new_metadata_length = old_metadata_length + 6;
    std::string old_metadata = slave.metadata;
    slave.metadata.resize(new_metadata_length);

    slave.metadata[1] = 1;
    // set variable E
    int E1 = slave.wcwr[0] - 3;
    int E2 = slave.wcwr[1] - 4;
    convert_dec_to_bin(E1, slave.metadata, old_metadata_length, 3);
    convert_dec_to_bin(E2, slave.metadata, old_metadata_length + 3, 3);
    return true;
}

void encode::updateSlaveMetadataE(int host_index, int slave_index)
{
    symbol *host = &symbols[host_index];
    symbol *slave = &symbols[slave_index];

    int offset = host->data.length();
    // find the start flag of metadata
    while (host->data[offset] == 0)
    {
        offset--;
    }
    // skip the flag bit
    offset--;
    // skip host metadata S
    if (host_index == 0)
        offset -= 4;
    else
        offset -= 3;
    // skip other slave symbol's metadata
    for (int i = 0; i < 4; i++)
    {
        if (host->slaves[i] == slave_index)
            break;
        else if (host->slaves[i] <= 0)
            continue;
        else
            offset -= symbols[host->slaves[i]].metadata.length();
    }
    // skip SS, SE and possibly V
    if (slave->metadata[0] == 1)
        offset -= 7;
    else
        offset -= 2;
    // update E
    std::string E;
    E.resize(6);
    int E1 = slave->wcwr[0] - 3;
    int E2 = slave->wcwr[1] - 4;
    convert_dec_to_bin(E1, E, 0, 3);
    convert_dec_to_bin(E2, E, 3, 3);
    for (int i = 0; i < 6; i++)
    {
        host->data[offset--] = E[i];
    }
}

bool encode::fitDataIntoSymbols()
{
    // calculate the net capacity of each symbol and the total net capacity
    int capacity[symbolNumber];
    int net_capacity[symbolNumber];
    int total_net_capacity = 0;
    for (int i = 0; i < symbolNumber; i++)
    {
        capacity[i] = getSymbolCapacity(i);
        symbols[i].wcwr[0] = ecclevel2wcwr[symbolEccLevels[i]][0];
        symbols[i].wcwr[1] = ecclevel2wcwr[symbolEccLevels[i]][1];
        net_capacity[i] = (capacity[i] / symbols[i].wcwr[1]) * symbols[i].wcwr[1] - (capacity[i] / symbols[i].wcwr[1]) * symbols[i].wcwr[0];
        total_net_capacity += net_capacity[i];
    }
    // assign data into each symbol
    int assigned_data_length = 0;
    for (int i = 0; i < symbolNumber; i++)
    {
        // divide data proportionally
        int s_data_length;
        if (i == symbolNumber - 1)
        {
            s_data_length = encodedData.length() - assigned_data_length;
        }
        else
        {
            float prop = (float)net_capacity[i] / (float)total_net_capacity;
            s_data_length = (int)(prop * encodedData.length());
        }
        int s_payload_length = s_data_length;

        // add flag bit
        s_payload_length++;
        // add host metadata S length (master metadata Part III or slave metadata Part III)
        if (i == 0)
            s_payload_length += 4;
        else
            s_payload_length += 3;
        // add slave metadata length
        for (int j = 0; j < 4; j++)
        {
            if (symbols[i].slaves[j] > 0)
            {
                s_payload_length += symbols[symbols[i].slaves[j]].metadata.length();
            }
        }

        // check if the full payload exceeds net capacity
        if (s_payload_length > net_capacity[i])
        {
            std::cout << "Message does not fit into the specified code. Use higher symbol version.";
            return false;
        }

        // add metadata E for slave symbols if free capacity available
        int j = 0;
        while (net_capacity[i] - s_payload_length >= 6 && j < 4)
        {
            if (symbols[i].slaves[j] > 0)
            {
                if (symbols[symbols[i].slaves[j]].metadata[1] == 0) // check SE
                {
                    if (!addE2SlaveMetadata(symbols[symbols[i].slaves[j]]))
                        return false;
                    s_payload_length += 6; // add E length
                }
            }
            j++;
        }

        // get optimal code rate
        int pn_length = s_payload_length;
        if (i == 0)
        {
            if (!isDefaultMode())
            {
                symbols[i].getOptimalECC(capacity[i], s_payload_length);
                pn_length = (capacity[i] / symbols[i].wcwr[1]) * symbols[i].wcwr[1] - (capacity[i] / symbols[i].wcwr[1]) * symbols[i].wcwr[0];
            }
            else
                pn_length = net_capacity[i];
        }
        else
        {
            if (symbols[i].metadata[1] == 1) // SE = 1
            {
                symbols[i].getOptimalECC(capacity[i], pn_length);
                pn_length = (capacity[i] / symbols[i].wcwr[1]) * symbols[i].wcwr[1] - (capacity[i] / symbols[i].wcwr[1]) * symbols[i].wcwr[0];
                updateSlaveMetadataE(symbols[i].host, i);
            }
            else
                pn_length = net_capacity[i];
        }

        // start to set full payload
        symbols[i].data.resize(pn_length);

        // set data
        symbols[i].data.assign(encodedData.substr(0, assigned_data_length), s_data_length);
        assigned_data_length += s_data_length;
        // set flag bit
        int set_pos = s_payload_length - 1;
        symbols[i].data[set_pos--] = 1;
        // set host metadata S
        for (int k = 0; k < 4; k++)
        {
            if (symbols[i].slaves[k] > 0)
            {
                symbols[i].data[set_pos--] = 1;
            }
            else if (symbols[i].slaves[k] == 0)
            {
                symbols[i].data[set_pos--] = 0;
            }
        }
        // set slave metadata
        for (int k = 0; k < 4; k++)
        {
            if (symbols[i].slaves[k] > 0)
            {
                for (int m = 0; m < symbols[symbols[i].slaves[k]].metadata.length(); m++)
                {
                    symbols[i].data[set_pos--] = symbols[symbols[i].slaves[k]].metadata[m];
                }
            }
        }
    }
    return true;
}

void encode::maskSymbols(int mask_type, int *masked)
{
    for (int k = 0; k < symbolNumber; k++)
    {
        int startx = 0, starty = 0;
        if (masked)
        {
            // calculate the starting coordinates of the symbol matrix
            int col = symbolPos[symbolPositions[k]].x - p.min_x;
            int row = symbolPos[symbolPositions[k]].y - p.min_y;
            for (int c = 0; c < col; c++)
                startx += p.col_width[c];
            for (int r = 0; r < row; r++)
                starty += p.row_height[r];
        }
        int symbol_width = symbols[k].side_size.x;
        int symbol_height = symbols[k].side_size.y;

        // apply mask on the symbol
        for (int y = 0; y < symbol_height; y++)
        {
            for (int x = 0; x < symbol_width; x++)
            {
                int index = symbols[k].matrix[y * symbol_width + x];
                if (symbols[k].dataMap[y * symbol_width + x])
                {
                    switch (mask_type)
                    {
                    case 0:
                        index ^= (x + y) % colorNumber;
                        break;
                    case 1:
                        index ^= x % colorNumber;
                        break;
                    case 2:
                        index ^= y % colorNumber;
                        break;
                    case 3:
                        index ^= (x / 2 + y / 3) % colorNumber;
                        break;
                    case 4:
                        index ^= (x / 3 + y / 2) % colorNumber;
                        break;
                    case 5:
                        index ^= ((x + y) / 2 + (x + y) / 3) % colorNumber;
                        break;
                    case 6:
                        index ^= ((x * x * y) % 7 + (2 * x * x + 2 * y) % 19) % colorNumber;
                        break;
                    case 7:
                        index ^= ((x * y * y) % 5 + (2 * x + y * y) % 13) % colorNumber;
                        break;
                    }
                    if (masked)
                        masked[(y + starty) * p.code_size.x + (x + startx)] = index;
                    else
                        symbols[k].matrix[y * symbol_width + x] = (uint8_t)index;
                }
                else
                {
                    if (masked)
                        masked[(y + starty) * p.code_size.x + (x + startx)] = index; // copy non-data module
                }
            }
        }
    }
}

bool encode::InitSymbols()
{
    // check all information for multi-symbol code are valid
    if (symbolNumber > 1)
    {
        for (int i = 0; i < symbolNumber; i++)
        {
            if (symbolVersions[i].x < 1 || symbolVersions[i].x > 32 || symbolVersions[i].y < 1 || symbolVersions[i].y > 32)
            {
                std::cout << "Incorrect symbol version for symbol " << i;
                return false;
            }
            if (symbolPositions[i] < 0 || symbolPositions[i] > MAX_SYMBOL_NUMBER)
            {
                std::cout << "Incorrect symbol position for symbol " << i;
                return false;
            }
        }
    }
    // move the master symbol to the first
    if (symbolNumber > 1 && symbolPositions[0] != 0)
    {
        for (int i = 0; i < symbolNumber; i++)
        {
            if (symbolPositions[i] == 0)
            {
                swap_int(&symbolPositions[i], &symbolPositions[0]);
                swap_int(&symbolVersions[i].x, &symbolVersions[0].x);
                swap_int(&symbolVersions[i].y, &symbolVersions[0].y);
                swap_byte(&symbolEccLevels[i], &symbolEccLevels[0]);
                break;
            }
        }
    }
    // if no master symbol exists in multi-symbol code
    if (symbolNumber > 1 && symbolPositions[0] != 0)
    {
        std::cout << "Master symbol missing";
        return false;
    }
    // if only one symbol but its position is not 0 - set to zero. Everything else makes no sense.
    if (symbolNumber == 1 && symbolPositions[0] != 0)
        symbolPositions[0] = 0;
    // check if a symbol position is used twice
    for (int i = 0; i < symbolNumber - 1; i++)
    {
        for (int j = i + 1; j < symbolNumber; j++)
        {
            if (symbolPositions[i] == symbolPositions[j])
            {
                std::cout << "Duplicate symbol position";
                return false;
            }
        }
    }
    // assign docked symbols to their hosts
    if (!assignDockedSymbols())
        return false;
    // check if the docked symbol size matches the docked side of its host
    if (!checkDockedSymbolSize())
        return false;
    // set symbol index and symbol side size
    for (int i = 0; i < symbolNumber; i++)
    {
        // set symbol index
        symbols[i].index = i;
        // set symbol side size
        symbols[i].side_size.x = VERSION2SIZE(symbolVersions[i].x);
        symbols[i].side_size.y = VERSION2SIZE(symbolVersions[i].y);
    }
    return true;
}

bool encode::setSlaveMetadata()
{
    // set slave metadata variables
    for (int i = 1; i < symbolNumber; i++)
    {
        int SS, SE, V, E1 = 0, E2 = 0;
        int metadata_length = 2; // Part I length
        // SS and V
        if (symbolVersions[i].x != symbolVersions[symbols[i].host].x)
        {
            SS = 1;
            V = symbolVersions[i].x - 1;
            metadata_length += 5;
        }
        else if (symbolVersions[i].y != symbolVersions[symbols[i].host].y)
        {
            SS = 1;
            V = symbolVersions[i].y - 1;
            metadata_length += 5;
        }
        else
        {
            SS = 0;
        }
        // SE and E
        if (symbolEccLevels[i] == 0 || symbolEccLevels[i] == symbolEccLevels[symbols[i].host])
        {
            SE = 0;
        }
        else
        {
            SE = 1;
            E1 = ecclevel2wcwr[symbolEccLevels[i]][0] - 3;
            E2 = ecclevel2wcwr[symbolEccLevels[i]][1] - 4;
            metadata_length += 6;
        }
        // write slave metadata
        symbols[i].metadata.resize(metadata_length);
        // Part I
        symbols[i].metadata[0] = SS;
        symbols[i].metadata[1] = SE;
        // Part II
        if (SS == 1)
        {
            convert_dec_to_bin(V, symbols[i].metadata, 2, 5);
        }
        if (SE == 1)
        {
            int start_pos = (SS == 1) ? 7 : 2;
            convert_dec_to_bin(E1, symbols[i].metadata, start_pos, 3);
            convert_dec_to_bin(E2, symbols[i].metadata, start_pos + 3, 3);
        }
    }
    return true;
}

void interleaveData(std::string &data)
{
    setSeed(INTERLEAVE_SEED);
    for (int i = 0; i < data.length(); i++)
    {
        int pos = (int)((float)lcg64_temper() / (float)UINT32_MAX * (data.length() - i));
        char tmp = data[data.length() - 1 - i];
        data[data.length() - 1 - i] = data[pos];
        data[pos] = tmp;
    }
}
int applyRule1(int *matrix, int width, int height, int color_number)
{
    uint8_t fp0_c1, fp0_c2;
    uint8_t fp1_c1, fp1_c2;
    uint8_t fp2_c1, fp2_c2;
    uint8_t fp3_c1, fp3_c2;
    if (color_number == 2) // two colors: black(000) white(111)
    {
        fp0_c1 = 0;
        fp0_c2 = 1;
        fp1_c1 = 1;
        fp1_c2 = 0;
        fp2_c1 = 1;
        fp2_c2 = 0;
        fp3_c1 = 1;
        fp3_c2 = 0;
    }
    else if (color_number == 4)
    {
        fp0_c1 = 0;
        fp0_c2 = 3;
        fp1_c1 = 1;
        fp1_c2 = 2;
        fp2_c1 = 2;
        fp2_c2 = 1;
        fp3_c1 = 3;
        fp3_c2 = 0;
    }
    else
    {
        fp0_c1 = FP0_CORE_COLOR;
        fp0_c2 = 7 - FP0_CORE_COLOR;
        fp1_c1 = FP1_CORE_COLOR;
        fp1_c2 = 7 - FP1_CORE_COLOR;
        fp2_c1 = FP2_CORE_COLOR;
        fp2_c2 = 7 - FP2_CORE_COLOR;
        fp3_c1 = FP3_CORE_COLOR;
        fp3_c2 = 7 - FP3_CORE_COLOR;
    }

    int score = 0;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            /*			//horizontal check
                        if(j + 4 < width)
                        {
                            if(matrix[i * width + j] 	 == fp0_c1 &&	//finder pattern 0
                               matrix[i * width + j + 1] == fp0_c2 &&
                               matrix[i * width + j + 2] == fp0_c1 &&
                               matrix[i * width + j + 3] == fp0_c2 &&
                               matrix[i * width + j + 4] == fp0_c1)
                               score++;
                            else if(									//finder pattern 1
                               matrix[i * width + j] 	 == fp1_c1 &&
                               matrix[i * width + j + 1] == fp1_c2 &&
                               matrix[i * width + j + 2] == fp1_c1 &&
                               matrix[i * width + j + 3] == fp1_c2 &&
                               matrix[i * width + j + 4] == fp1_c1)
                               score++;
                            else if(									//finder pattern 2
                               matrix[i * width + j] 	 == fp2_c1 &&
                               matrix[i * width + j + 1] == fp2_c2 &&
                               matrix[i * width + j + 2] == fp2_c1 &&
                               matrix[i * width + j + 3] == fp2_c2 &&
                               matrix[i * width + j + 4] == fp2_c1)
                               score++;
                            else if(									//finder pattern 3
                               matrix[i * width + j] 	 == fp3_c1 &&
                               matrix[i * width + j + 1] == fp3_c2 &&
                               matrix[i * width + j + 2] == fp3_c1 &&
                               matrix[i * width + j + 3] == fp3_c2 &&
                               matrix[i * width + j + 4] == fp3_c1)
                               score++;
                        }
                        //vertical check
                        if(i + 4 < height)
                        {
                            if(matrix[i * width + j] 	   == fp0_c1 &&	//finder pattern 0
                               matrix[(i + 1) * width + j] == fp0_c2 &&
                               matrix[(i + 2) * width + j] == fp0_c1 &&
                               matrix[(i + 3) * width + j] == fp0_c2 &&
                               matrix[(i + 4) * width + j] == fp0_c1)
                               score++;
                            else if(									//finder pattern 1
                               matrix[i * width + j] 	   == fp1_c1 &&
                               matrix[(i + 1) * width + j] == fp1_c2 &&
                               matrix[(i + 2) * width + j] == fp1_c1 &&
                               matrix[(i + 3) * width + j] == fp1_c2 &&
                               matrix[(i + 4) * width + j] == fp1_c1)
                               score++;
                            else if(									//finder pattern 2
                               matrix[i * width + j] 	   == fp2_c1 &&
                               matrix[(i + 1) * width + j] == fp2_c2 &&
                               matrix[(i + 2) * width + j] == fp2_c1 &&
                               matrix[(i + 3) * width + j] == fp2_c2 &&
                               matrix[(i + 4) * width + j] == fp2_c1)
                               score++;
                            else if(									//finder pattern 3
                               matrix[i * width + j] 	   == fp3_c1 &&
                               matrix[(i + 1) * width + j] == fp3_c2 &&
                               matrix[(i + 2) * width + j] == fp3_c1 &&
                               matrix[(i + 3) * width + j] == fp3_c2 &&
                               matrix[(i + 4) * width + j] == fp3_c1)
                               score++;
                        }
            */
            if (j >= 2 && j <= width - 3 && i >= 2 && i <= height - 3)
            {
                if (matrix[i * width + j - 2] == fp0_c1 && // finder pattern 0
                    matrix[i * width + j - 1] == fp0_c2 &&
                    matrix[i * width + j] == fp0_c1 &&
                    matrix[i * width + j + 1] == fp0_c2 &&
                    matrix[i * width + j + 2] == fp0_c1 &&
                    matrix[(i - 2) * width + j] == fp0_c1 &&
                    matrix[(i - 1) * width + j] == fp0_c2 &&
                    matrix[(i)*width + j] == fp0_c1 &&
                    matrix[(i + 1) * width + j] == fp0_c2 &&
                    matrix[(i + 2) * width + j] == fp0_c1)
                    score++;
                else if (
                    matrix[i * width + j - 2] == fp1_c1 && // finder pattern 1
                    matrix[i * width + j - 1] == fp1_c2 &&
                    matrix[i * width + j] == fp1_c1 &&
                    matrix[i * width + j + 1] == fp1_c2 &&
                    matrix[i * width + j + 2] == fp1_c1 &&
                    matrix[(i - 2) * width + j] == fp1_c1 &&
                    matrix[(i - 1) * width + j] == fp1_c2 &&
                    matrix[(i)*width + j] == fp1_c1 &&
                    matrix[(i + 1) * width + j] == fp1_c2 &&
                    matrix[(i + 2) * width + j] == fp1_c1)
                    score++;
                else if (
                    matrix[i * width + j - 2] == fp2_c1 && // finder pattern 2
                    matrix[i * width + j - 1] == fp2_c2 &&
                    matrix[i * width + j] == fp2_c1 &&
                    matrix[i * width + j + 1] == fp2_c2 &&
                    matrix[i * width + j + 2] == fp2_c1 &&
                    matrix[(i - 2) * width + j] == fp2_c1 &&
                    matrix[(i - 1) * width + j] == fp2_c2 &&
                    matrix[(i)*width + j] == fp2_c1 &&
                    matrix[(i + 1) * width + j] == fp2_c2 &&
                    matrix[(i + 2) * width + j] == fp2_c1)
                    score++;
                else if (
                    matrix[i * width + j - 2] == fp3_c1 && // finder pattern 3
                    matrix[i * width + j - 1] == fp3_c2 &&
                    matrix[i * width + j] == fp3_c1 &&
                    matrix[i * width + j + 1] == fp3_c2 &&
                    matrix[i * width + j + 2] == fp3_c1 &&
                    matrix[(i - 2) * width + j] == fp3_c1 &&
                    matrix[(i - 1) * width + j] == fp3_c2 &&
                    matrix[(i)*width + j] == fp3_c1 &&
                    matrix[(i + 1) * width + j] == fp3_c2 &&
                    matrix[(i + 2) * width + j] == fp3_c1)
                    score++;
            }
        }
    }
    return W1 * score;
}

int applyRule2(int *matrix, int width, int height)
{
    int score = 0;
    for (int i = 0; i < height - 1; i++)
    {
        for (int j = 0; j < width - 1; j++)
        {
            if (matrix[i * width + j] != -1 && matrix[i * width + (j + 1)] != -1 &&
                matrix[(i + 1) * width + j] != -1 && matrix[(i + 1) * width + (j + 1)] != -1)
            {
                if (matrix[i * width + j] == matrix[i * width + (j + 1)] &&
                    matrix[i * width + j] == matrix[(i + 1) * width + j] &&
                    matrix[i * width + j] == matrix[(i + 1) * width + (j + 1)])
                    score++;
            }
        }
    }
    return W2 * score;
}

int applyRule3(int *matrix, int width, int height)
{
    int score = 0;
    for (int k = 0; k < 2; k++)
    {
        int maxi, maxj;
        if (k == 0) // horizontal scan
        {
            maxi = height;
            maxj = width;
        }
        else // vertical scan
        {
            maxi = width;
            maxj = height;
        }
        for (int i = 0; i < maxi; i++)
        {
            int same_color_count = 0;
            int pre_color = -1;
            for (int j = 0; j < maxj; j++)
            {
                int cur_color = (k == 0 ? matrix[i * width + j] : matrix[j * width + i]);
                if (cur_color != -1)
                {
                    if (cur_color == pre_color)
                        same_color_count++;
                    else
                    {
                        if (same_color_count >= 5)
                            score += W3 + (same_color_count - 5);
                        same_color_count = 1;
                        pre_color = cur_color;
                    }
                }
                else
                {
                    if (same_color_count >= 5)
                        score += W3 + (same_color_count - 5);
                    same_color_count = 0;
                    pre_color = -1;
                }
            }
            if (same_color_count >= 5)
                score += W3 + (same_color_count - 5);
        }
    }
    return score;
}

int evaluateMask(int *matrix, int width, int height, int color_number)
{
    return applyRule1(matrix, width, height, color_number) + applyRule2(matrix, width, height) + applyRule3(matrix, width, height);
}

int encode::maskCode()
{
    int mask_type = 0;
    int min_penalty_score = 10000;

    // allocate memory for masked code
    int *masked = (int *)malloc(p.code_size.x * p.code_size.y * sizeof(int));
    if (masked == NULL)
    {
        std::cout << "Memory allocation for masked code failed";
        return -1;
    }
    memset(masked, -1, p.code_size.x * p.code_size.y * sizeof(int)); // set all bytes in masked as 0xFF

    // evaluate each mask pattern
    for (int t = 0; t < NUMBER_OF_MASK_PATTERNS; t++)
    {
        int penalty_score = 0;
        maskSymbols(t, masked);
        // calculate the penalty score
        penalty_score = evaluateMask(masked, p.code_size.x, p.code_size.y, colorNumber);

        if (penalty_score < min_penalty_score)
        {
            mask_type = t;
            min_penalty_score = penalty_score;
        }
    }

    // mask all symbols with the selected mask pattern
    maskSymbols(mask_type, 0);

    // clean memory
    free(masked);
    return mask_type;
}

int encode::generateChromaCode(std::string &data)
{
    // Check data
    if (data.empty())
    {
        std::cout << "No input data specified!";
        return 2;
    }
    if (data.length() == 0)
    {
        std::cout << "No input data specified!";
        return 2;
    }

    // initialize symbols and set metadata in symbols
    if (!InitSymbols())
        return 3;

    // get the optimal encoded length and encoding sequence
    encodeSeq = analyzeInputData(data);
    if (encodeSeq.empty())
    {
        std::cout << "Analyzing input data failed";
        return 1;
    }
    // encode data using optimal encoding modes
    encodeData(data);

    // set master symbol version if not given
    if (symbolNumber == 1 && (symbolVersions[0].x == 0 || symbolVersions[0].y == 0))
    {
        if (!setMasterSymbolVersion())
        {
            encodedData.clear();
            return 4;
        }
    }
    // set metadata for slave symbols
    if (!setSlaveMetadata())
    {
        encodedData.clear();
        return 1;
    }
    // assign encoded data into symbols
    if (!fitDataIntoSymbols())
    {
        encodedData.clear();
        return 4;
    }
    encodedData.clear();
    // set master metadata
    if (!isDefaultMode())
    {
        if (!encodeMasterMetadata())
        {
            std::cout << "Encoding master symbol metadata failed";
            return 1;
        }
    }

    // encode each symbol in turn
    for (int i = 0; i < symbolNumber; i++)
    {
        std::vector<int> swcwr{symbols[i].wcwr[0], symbols[i].wcwr[1]};
        // error correction for data
        std::string ecc_encoded_data = encodeLDPC(symbols[i].data, swcwr);
        if (ecc_encoded_data.empty())
        {
            std::cout << "LDPC encoding for the data in symbol %d failed" << i;
            return 1;
        }
        // interleave
        interleaveData(ecc_encoded_data);
        // create Matrix
        bool cm_flag = createMatrix(i, ecc_encoded_data);
        ecc_encoded_data.clear();
        if (!cm_flag)
        {
            std::cout << "Creating matrix for symbol " << i << " failed";
            return 1;
        }
    }

    // mask all symbols in the code
    getCodePara();

    if (isDefaultMode()) // default mode
    {
        maskSymbols(DEFAULT_MASKING_REFERENCE, 0);
    }
    else
    {
        int mask_reference = maskCode();
        if (mask_reference < 0)
        {
            return 1;
        }

        if (mask_reference != DEFAULT_MASKING_REFERENCE)
        {
            // re-encode PartII of master symbol metadata
            updateMasterMetadataPartII(mask_reference);
            // update the masking reference in master symbol metadata
            placeMasterMetadataPartII();
        }
    }

    // create the code bitmap
    bool cb_flag = createBitmap();

    if (!cb_flag)
    {
        std::cout << "Creating the code bitmap failed";
        return 1;
    }
    return 0;
}

int main()
{
    return 0;
}