#ifndef LDPC_H
#define LDPC_H

#include <cstdint>
#include <string>
#include <vector>
#define LPDC_METADATA_SEED 38545
#define LPDC_MESSAGE_SEED 785465

std::string encodeLDPC(std::string data, std::vector<int32_t> coderate_params);

#endif
