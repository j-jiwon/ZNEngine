#pragma once
#include "ZNApplication.h"
#include "Platform/ApplicationPlatform.h"
#include <cstdlib>
#include <exception>
#include <iostream>

using namespace ZNFramework;

ZNApplication::ZNApplication()
    :context(nullptr)
{
    context = CreateContext();
}

ZNApplication::~ZNApplication()
{
    delete context;
    context = nullptr;
}

int ZNApplication::Run()
{
    bool terminated = false;
    const auto terminateOnce = [this, &terminated]() {
        if (!terminated)
        {
            terminated = true;
            OnTerminate();
        }
    };

    try
    {
        OnInitialize();
        const int exitcode = context->MessageLoop();
        terminateOnce();
        return exitcode;
    }
    catch (const std::exception& error)
    {
        std::cerr << "application failure: " << error.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "application failure: unknown exception" << std::endl;
    }

    try
    {
        terminateOnce();
    }
    catch (const std::exception& error)
    {
        std::cerr << "application cleanup failure: " << error.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "application cleanup failure: unknown exception" << std::endl;
    }
    return EXIT_FAILURE;
}
