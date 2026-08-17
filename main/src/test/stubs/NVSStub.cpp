#include "test/stubs/NVSStub.h"
#include <cstring>

NVSStub::NVSStub(NVSStub&& other) noexcept :
    INVS(std::move(other)),
    lastNamespace(std::move(other.lastNamespace)),
    beginCallCount(other.beginCallCount),
    intValues(std::move(other.intValues)),
    stringValues(std::move(other.stringValues))
{
    other.beginCallCount = 0;
}

bool NVSStub::begin(const char* namespaceName) noexcept {
    if (isInitialized) {
        return false;
    }

    lastNamespace = namespaceName;
    ++beginCallCount;
    isInitialized = true;
    return true;
}

bool NVSStub::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    isInitialized = false;
    return true;
}

bool NVSStub::setInt(const char* key, int32_t value) noexcept {
    if (!isInitialized) {
        return false;
    }

    intValues[key] = value;
    return true;
}

bool NVSStub::getInt(const char* key, int32_t& outValue) const noexcept {
    if (!isInitialized || !intValues.contains(key)) {
        return false;
    }

    outValue = intValues.at(key);
    return true;
}

bool NVSStub::setString(const char* key, const char* value) noexcept {
    if (!isInitialized || std::strlen(value) > NVS_MAX_STRING_LENGTH) {
        return false;
    }

    stringValues[key] = value;
    return true;
}

bool NVSStub::getString(const char* key, char* outValue) const noexcept {
    if (!isInitialized || !stringValues.contains(key)) {
        return false;
    }

    std::strcpy(outValue, stringValues.at(key).c_str());
    return true;
}

bool NVSStub::eraseKey(const char* key) noexcept {
    if (!isInitialized) {
        return false;
    }

    intValues.erase(key);
    stringValues.erase(key);
    return true;
}

const std::string& NVSStub::getLastNamespace() const {
    return lastNamespace;
}

int NVSStub::getBeginCallCount() const {
    return beginCallCount;
}
