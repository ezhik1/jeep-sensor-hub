#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

void boot_screen_init(void);
void boot_screen_update_progress(int progress);
void boot_screen_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // BOOT_SCREEN_H
