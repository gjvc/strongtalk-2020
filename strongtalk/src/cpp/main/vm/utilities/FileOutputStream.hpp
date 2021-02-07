//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/utilities/ConsoleOutputStream.hpp"


class FileOutputStream : public ConsoleOutputStream {

protected:
    std::ofstream _file;

public:
    FileOutputStream( const char *file_name );
    ~FileOutputStream();


    int32_t is_open() const {
        return _file.good();
    }


    void put( char c );
};


FileOutputStream::FileOutputStream( const char *file_name ) : ConsoleOutputStream(), _file{} {
    _file.open( file_name );
}


void FileOutputStream::put( char c ) {
    _file.put( c );
    _position++;
}


FileOutputStream::~FileOutputStream() {
    _file.close();
}
