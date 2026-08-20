/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#ifndef QASC_COLLECT_ERROR_STRATEGY_H
#define QASC_COLLECT_ERROR_STRATEGY_H

#include "DefaultErrorStrategy.h"

class CollectErrorStrategy : public antlr4::DefaultErrorStrategy {
   public:
    CollectErrorStrategy();

    CollectErrorStrategy(CollectErrorStrategy const& other) = delete;

    virtual ~CollectErrorStrategy();

    CollectErrorStrategy& operator=(CollectErrorStrategy const& other) = delete;

    void beginErrorCondition(antlr4::Parser* recognizer) override;

    void reportErrors() const;

   private:
    int numErrors_{0};
};

#endif    // !QASC_COLLECT_ERROR_STRATEGY_H
