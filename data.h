#pragma once

#include <memory>

//----------------------------------------------------------------------
// Data
//----------------------------------------------------------------------

class Data {
public:
    virtual ~Data() = default;
};

using DataPtr = std::shared_ptr<Data>;

