
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <string>
#include <iostream>

#include "vm/platform/platform.hpp"


// ----------------------------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
class ConfigurationValue {
public:
    std::string _name;
    T           _value;
    std::string _description;
    T           _default_value;

public:
    explicit ConfigurationValue( const T &value ) :
        _name{ "" },
        _value{ value },
        _default_value{},
        _description{ "" } {}


    ConfigurationValue( const char *name, const T &value ) :
        _name{ name },
        _value{ value },
        _default_value{},
        _description{ "" } {}


    ConfigurationValue( const char *name, const T &value, const char *description ) :
        _name{ name },
        _value{ value },
        _description{ description },
        _default_value{} {}


    ConfigurationValue &operator=( const ConfigurationValue &other ) {
        if ( &other == this ) {
            return *this;
        }

        _name        = other._name;
        _value       = other._value;
        _description = other._description;

        return *this;
    }


    const std::string &name() { return _name; }


    const std::string &description() { return _description; }


    const T value() { return _value; }


    const T default_value() { return _default_value; }


    operator T() const { return _value; }


};


// ----------------------------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
class ConfigurationOverride {
public:
    ConfigurationValue<T> &_cv;
    T                     _default_value;
    T                     _value;

public:
    ConfigurationOverride( ConfigurationValue<T> &configuration_value, const T &default_value ) :
        _cv{ configuration_value },
        _default_value{ default_value },
        _value{} {
        st_unused( default_value ); // unused
    }


    ~ConfigurationOverride() {
        _cv._value = _default_value;
    }


    const T &default_value() { return _default_value; }


    const T &value() { return _value; }


    operator T() const { return _value; }


};


// ----------------------------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
constexpr T _flag( const char *name, T default_value, const char *description ) {

    auto cv = ConfigurationValue<T>( name, default_value, description );

    if constexpr ( std::is_same<T, bool>::value ) {
        auto co = ConfigurationOverride<T>( cv, not default_value );
        SPDLOG_INFO( "bool [{}], default value [{}], overridden value [{}]", cv.name(), cv.value(), co.value() );
    }

    if constexpr ( std::is_same<T, std::int32_t>::value ) {
        auto co = ConfigurationOverride<T>( cv, default_value + 1 );
        SPDLOG_INFO( "std::int32_t [{}], default value [{}], overridden value [{}]", cv.name(), cv.value(), co.value() );
    }

    return T( default_value );
}
