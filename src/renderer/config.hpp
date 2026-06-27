#pragma once

namespace tk::renderer {

constexpr auto Frame_Count = 2;

constexpr auto Buffer_Init_Size = 1024;

constexpr auto CBV_SRV_UAV_Heap_Size = 256;
constexpr auto RTV_Heap_Size         = 256;
constexpr auto dSV_Heap_Size         = 32;

constexpr auto Window_Resize_Thickness           = 10;
constexpr auto Window_Shadow_Thickness           = 20;
constexpr auto Window_Y_Pos_Moving_From_Maximize = 10;
constexpr auto Window_Min_Width                  = 50;
constexpr auto Window_Min_Height                 = 50;

constexpr auto Image_Pool_Init_Capacity = 64;

constexpr auto Mouse_Left_Down_Press_Start_Time = 400;

constexpr auto Default_Image_Init_Width  = 512;
constexpr auto Default_Image_Init_Height = 512;

constexpr auto Cancel_Vsync_Duration = 200'000;

}
