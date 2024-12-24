#pragma once
#include "ZNApplication.h"
#include "Platform/ApplicationPlatform.h"

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
    OnInitialize();
    // process message loop - exit when exitcode is 0
    int exitcode = context->MessageLoop();

    OnTerminate();
    return exitcode;
}
