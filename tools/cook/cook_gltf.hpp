#pragma once

#include <string>

bool cook_glb_to_wbmesh(const char* glb_path, const char* out_wbmesh, std::string& err_msg);

bool write_white_texture_rgba8(const char* out_path);
