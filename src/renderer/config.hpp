#pragma once

namespace tk { namespace renderer {

constexpr auto Frame_Count                      = 2;

constexpr auto Vertices_Indices_Buffer_Size     = 1024;
constexpr auto Shape_Properties_Buffer_Size     = 1024;

constexpr auto CBV_SRV_UAV_Heap_Size            = 256;
constexpr auto RTV_Heap_Size                    = 256;
constexpr auto dSV_Heap_Size                    = 32;

constexpr auto Window_Resize_Width              = 5;
constexpr auto Window_Resize_Height             = 5;
constexpr auto Window_Shadow_Thickness          = 20;

constexpr auto Enable_Depth_Test                = false;

constexpr auto Image_Pool_Init_Capacity         = 32;

constexpr auto Renderer_Msg_Queue_Capacity      = 16;
constexpr auto Render_Data_Queue_Capacity       = 32;

constexpr auto Mouse_Left_Down_Press_Start_Time = 400;

}}
