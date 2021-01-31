#ifndef GAME_CONFIG_H_INCLUDED
#define GAME_CONFIG_H_INCLUDED

#include "preferences.hpp"
#include "sdl_utils.hpp"

#define DEFAULT_SUPRESS_THRESHOLD		1800 // 30 minite
#define DEFAULT_SN		"kos-device"

namespace game_config {

extern int max_fps_to_encoder;
extern std::map<int, std::string> suppress_thresholds;
extern version_info kosapi_ver;
extern void* explorer_singleton;
extern SDL_threadID rdpd_tid;
}

void VALIDATE_IN_RDPD_THREAD();

namespace preferences {

std::string sn();
void set_sn(const std::string& value);
bool bleperipheral();
void set_bleperipheral(bool value);

}

#endif

