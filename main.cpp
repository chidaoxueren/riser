//
//  main.cpp
//  riser
//
//  Created by wyt on 13-1-20.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include <iostream>
#include "gent_frame.h"

int main(int argc, const char * argv[])
{

    // insert code here...
    std::cout << "Hello, World!\n";
    //GentFrame::Instance()->Run(atoi(argv[1]));
    if(!GentFrame::Instance()->Init()){
        std::cout << "init fail!\n";
        return 1;
    }
    GentFrame::Instance()->Run(10);
    return 0;
}

