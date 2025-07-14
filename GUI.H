#ifndef __GUI_H__
#define __GUI_H__

#define GUI_KEY_VOLUME_UP 'U'
#define GUI_KEY_VOLUME_DOWN 'Y'
#define GUI_KEY_NEXT 'O'
#define GUI_KEY_PREV 'I'
#define GUI_KEY_PAUSE 'P'
#define GUI_KEY_EXIT 27

int gui_init(const char *path);
void gui_deinit(void);

int gui_task(void);

#endif
