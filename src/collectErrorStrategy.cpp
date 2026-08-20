/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#include "collectErrorStrategy.h"
#include <format>
#include <stdexcept>

CollectErrorStrategy::CollectErrorStrategy() {
    errorRecoveryMode = false;
    lastErrorIndex = -1;
    numErrors_ = 0;
}

CollectErrorStrategy::~CollectErrorStrategy() {
}

void CollectErrorStrategy::beginErrorCondition(antlr4::Parser* recognizer) {
    errorRecoveryMode = true;
    numErrors_++;
}

void CollectErrorStrategy::reportErrors() const {
    if (numErrors_ > 0) {
        throw std::runtime_error(
            std::format("Encountered {} errors while parsing.", numErrors_));
    }
}
