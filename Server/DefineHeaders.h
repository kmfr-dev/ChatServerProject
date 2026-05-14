#pragma once

// Windows.h 에는 WinSock.h(WinSock2.h의 구버전)이 포함되어 있어 중복 정의가 발생.

#define WIN32_LEAN_AND_MEAN

#include <d3d11.h>
#include <iostream>
#include <vector>
#include <string>

#include <WinSock2.h>
#include <thread>

#include "imgui.h"
#include "crtdbg.h"
