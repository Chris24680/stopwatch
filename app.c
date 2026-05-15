#include "pala_app.h"
#include "pala_api.h"

__attribute__((section(".header")))
const PalaAppHeader pala_header = {
    .magic        = PALA_APP_MAGIC,
    .api_version  = PALA_API_VERSION,
    .name         = "Stopwatch",
    .entry_offset = 0,
    .reloc_offset = 0,
    .reloc_count  = 0,
};

#define MAX_LAPS      4
#define UPDATE_MS     100

static void format_time(const PalaAPI* api, char* buf, int len, uint32_t ms) {
    uint32_t s = ms / 1000;
    uint32_t t = (ms % 1000) / 100;
    uint32_t m = s / 60;
    s = s % 60;
    if (m >= 60) {
        uint32_t h = m / 60;
        m = m % 60;
        api->snprintf_wrap(buf, len, "%lu:%02lu:%02lu", h, m, s);
    } else {
        api->snprintf_wrap(buf, len, "%02lu:%02lu.%lu", m, s, t);
    }
}

static void draw_ui(const PalaAPI* api, uint32_t cur_ms, int running,
                    uint32_t* laps, int lap_count) {
    char buf[24];

    api->clearScreen();
    api->drawHeader("Stopwatch");

    format_time(api, buf, sizeof(buf), cur_ms);
    api->drawCenteredLarge(buf);

    for (int i = 0; i < lap_count; i++) {
        char lap_buf[32];
        char time_buf[16];
        format_time(api, time_buf, sizeof(time_buf), laps[i]);
        api->snprintf_wrap(lap_buf, sizeof(lap_buf), "LAP %d  %s", i + 1, time_buf);
        api->drawTextAt(6, 70 + i * 12, lap_buf, 0);
    }

    const char* status = running ? "RUNNING" : (cur_ms > 0 ? "PAUSED" : "");
    api->drawTextAt(6, 114, status, 0);

    api->refreshDisplay();
}

void app_main(const PalaAPI* api) {
    uint32_t elapsed_ms = 0;
    uint32_t run_start  = 0;
    int      running    = 0;
    uint32_t laps[MAX_LAPS];
    int      lap_count  = 0;
    uint32_t last_draw  = 0;
    int      need_redraw = 1;

    for (int i = 0; i < MAX_LAPS; i++) laps[i] = 0;

    while (1) {
        uint32_t now = api->millisNow();
        uint32_t cur = running ? elapsed_ms + (now - run_start) : elapsed_ms;

        uint8_t evt = api->pollEvent();
        if (evt == PALA_CLICK) {
            if (!running) {
                run_start = now;
                running   = 1;
            } else {
                elapsed_ms += now - run_start;
                running    = 0;
            }
            need_redraw = 1;
        } else if (evt == PALA_DOUBLE) {
            if (running && lap_count < MAX_LAPS) {
                laps[lap_count++] = cur;
                need_redraw = 1;
            }
        } else if (evt == PALA_TRIPLE) {
            return;
        }

        if (running && (uint32_t)(now - last_draw) >= UPDATE_MS) {
            need_redraw = 1;
        }

        if (need_redraw) {
            draw_ui(api, cur, running, laps, lap_count);
            last_draw  = now;
            need_redraw = 0;
        }

        api->delayMs(10);
    }
}
