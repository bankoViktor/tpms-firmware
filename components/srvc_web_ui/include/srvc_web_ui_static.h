/**
 ********************************************************************************
 * @file    srvc_web_ui_static.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    18.03.2025
 * @brief   Header file of the Web UI static fiels.
 ********************************************************************************
 */

#ifndef SRVC_WEB_UI_STATIC__H
#define SRVC_WEB_UI_STATIC__H

#define MIME_ICO "image/vnd.microsoft.icon"
#define MIME_PNG "image/png"
#define MIME_HTML "text/html"
#define MIME_CSS "text/css"
#define MIME_JS "text/javascript"
#define MIME_JSON "application/json"
#define MIME_GZ "application/gzip"

typedef struct web_static_map_entity_t {
  const char *alias;
  const char *mime_type;
  const char *start;
  const char *end;
} web_static_map_entity_t;

#define WEB_FILE_DECLR(sym)                                                    \
  extern const char _binary##sym##_start[];                                    \
  extern const char _binary##sym##_end[];

#define WEB_STATIC_MAP_BEGIN()                                                 \
  const web_static_map_entity_t _web_static_map[] = {

#define WEB_STATIC_MAP_ENTITY(sym, mime)                                       \
  {#sym, mime, _binary##sym##_start, _binary##sym##_end},

#define WEB_STATIC_MAP_END()                                                   \
  }                                                                            \
  ;

#endif
