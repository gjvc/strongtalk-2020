
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once



class Console {

public:
    template<typename... Args>
    void info( const char *fmt, Args &... args );


    void info( const char *msg ) {
        this->info( "{}", msg );
    }

};
