/*

*/

#pragma once

#include <string>

namespace caspar { namespace ffmpeg {

void init();
void uninit();
std::shared_ptr<void> temporary_enable_quiet_logging_for_thread(bool enable);
void enable_quiet_logging_for_thread();
bool is_logging_quiet_for_thread();

}}
