//
// Created by aidoo on 2024/10/31.
//

#ifndef _APP_H
#define _APP_H

#include <string>

namespace libRetroRunner {

    class AppContext {


    public:
        AppContext();

        ~AppContext();

    public:
        static AppContext *CreateInstance();

        static AppContext *Current() {
            return instance.get();
        }

    private:

    };
    
}


#endif
